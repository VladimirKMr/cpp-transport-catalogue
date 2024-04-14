#include "json_reader.h"
#include "transport_catalogue.h"
#include "geo.h"
#include "json.h"
#include "json_builder.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace json_reader {

    // вспомогательный метод для вывода маршрутов по запросу Stop, в формате json
    json::Dict JsonReader::StopResponseToJsonDict(int request_id, const std::optional<domain::StopInfo>& stop_info) const {
        
        json::Builder stop_build;

        stop_build.StartDict();
        if (stop_info) {
            stop_build.Key("buses"s).StartArray();
            for (const auto& bus : stop_info->buses) {
                stop_build.Value(std::string(bus));
            }
            stop_build.EndArray();
        }
        else {
            stop_build.Key("error_message"s).Value("not found"s);
        }
        stop_build.Key("request_id"s).Value(request_id);
        stop_build.EndDict();

        return stop_build.Build().AsDict();
    }

    // вспомогательный метод для вывода информации о маршруте по запросу Bus, в формате json
    json::Dict JsonReader::BusResponseToJsonDict(int request_id, const std::optional<domain::BusInfo>& bus_info) const {
        
        json::Builder bus_build;

        bus_build.StartDict();
        if (bus_info) {
            bus_build.Key("curvature"s).Value(bus_info->curvature)
                     .Key("route_length"s).Value(bus_info->route_length)
                     .Key("stop_count"s).Value((int)bus_info->stops)
                     .Key("unique_stop_count"s).Value((int)bus_info->uniq_stops);
        }
        else {
            bus_build.Key("error_message"s).Value("not found"s);
        }
        bus_build.Key("request_id"s).Value(request_id);
        bus_build.EndDict();

        return bus_build.Build().AsDict();
    }

    // вспомогательный метод для вывода svg рендера по запросу Map
    json::Dict JsonReader::MapResponseToJsonDict(int request_id, const svg::Document& render_doc) const {
        
        json::Builder map_build;

        std::ostringstream out;
        render_doc.Render(out);
        std::string svg_out = out.str();

        map_build.StartDict()
                 .Key("map"s).Value(std::move(svg_out))
                 .Key("request_id"s).Value(request_id)
                 .EndDict();

        return map_build.Build().AsDict();
    }

    // вспомогательный метод для вывода информации по запросу Route (выбор маршрута)
    json::Dict JsonReader::RouteResponseToJsonDict(int request_id, 
                                                   const std::optional<graph::Router<double>::RouteInfo>& routing,
                                                   const graph::DirectedWeightedGraph<double>& graph) const {
        json::Builder route_build;

        if (!routing) {
            route_build.StartDict()
                       .Key("request_id"s).Value(request_id)
                       .Key("error_message"s).Value("not found"s)
                       .EndDict();
        }
        else {
            json::Array arr_route;
            double time_route = 0.0;
            arr_route.reserve(routing.value().edges.size());

            for (auto& edge_id : routing.value().edges) {
                const graph::Edge<double> edge = graph.GetEdge(edge_id);

                if (edge.quality == 0) {
                    arr_route.emplace_back(json::Node(json::Builder{}
                             .StartDict()
                             .Key("stop_name"s).Value(edge.name)
                             .Key("time"s).Value(edge.weight)
                             .Key("type"s).Value("Wait"s)
                             .EndDict().Build()));

                    time_route += edge.weight;
                }
                else {
                    arr_route.emplace_back(json::Node(json::Builder{}
                             .StartDict()
                             .Key("bus"s).Value(edge.name)
                             .Key("span_count"s).Value(static_cast<int>(edge.quality))
                             .Key("time"s).Value(edge.weight)
                             .Key("type"s).Value("Bus"s)
                             .EndDict().Build()));

                    time_route += edge.weight;
                }
            }

            route_build.StartDict()
                       .Key("request_id"s).Value(request_id)
                       .Key("total_time"s).Value(time_route)
                       .Key("items"s).Value(arr_route)
                       .EndDict();
        }
        
        return route_build.Build().AsDict();
    }

    // обработка запросов к каталогу и вывод информации с помощью вспомогательных методов конвертации в json
    void JsonReader::ResponseRequests(std::ostream& os, const transport::RequestHandler& rq) const {
        
        json::Builder responses;

        responses.StartArray();
        for (const auto& request : json_document_.GetRoot().AsDict().at("stat_requests"s).AsArray()) {

            const auto& request_id = request.AsDict().at("id"s).AsInt();

            if (request.AsDict().at("type"s).AsString() == "Stop"sv) {
                auto result = rq.GetBusesByStop(request.AsDict().at("name"s).AsString());
                responses.Value(StopResponseToJsonDict(request_id, result));
            }

            else if (request.AsDict().at("type"s).AsString() == "Bus"sv) {
                auto result = rq.GetBusInfo(request.AsDict().at("name"s).AsString());
                responses.Value(BusResponseToJsonDict(request_id, result));
            }

            else if (request.AsDict().at("type"s).AsString() == "Map"sv) {
                responses.Value(MapResponseToJsonDict(request_id, rq.RenderMap()));
            }

            else if (request.AsDict().at("type"s).AsString() == "Route"sv) {
                const std::string_view stop_from = request.AsDict().at("from"s).AsString();
                const std::string_view stop_to = request.AsDict().at("to"s).AsString();
                const auto& routing = rq.GetOptimalRoute(stop_from, stop_to);
                const auto& route_graph = rq.GetRouterGraph();
                responses.Value(RouteResponseToJsonDict(request_id, routing, route_graph));
            }
        }

        responses.EndArray();
        json::Print(json::Document(responses.Build()), os);
    }

    // вспомогательный метод для заполнения каталога, забирает информацию об остановке
    Stop JsonReader::ParseStopQuery(const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsDict();
        return Stop{ type_stop_map.at("name"s).AsString(),
                        geo::Coordinates{type_stop_map.at("latitude"s).AsDouble(),
                                         type_stop_map.at("longitude"s).AsDouble()} };
    }

    // вспомогательный метод для заполнения каталога, забирает информацию о дистанциях между остановками
    void JsonReader::ParseStopQueryDistance(transport::TransportCatalogue& ts, const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsDict();
        for (const auto& [to_stop, dist_to_stop] : type_stop_map.at("road_distances"s).AsDict()) {
            ts.AddStopPairDistances(ts.FindStop(type_stop_map.at("name"s).AsString()),
                                    ts.FindStop(to_stop), dist_to_stop.AsInt());
        }

    }

    // вспомогательный метод для заполнения каталога, добавляет остановки в маршрут в соответствии с флагом is_roundtrip (кольцевой или нет)
    Bus JsonReader::ParseBusQuery(const transport::TransportCatalogue& ts, const json::Node& node_bus) const {
        const auto& node_bus_map = node_bus.AsDict();
        Bus bus;
        bus.is_roundtrip = node_bus_map.at("is_roundtrip"s).AsBool();
        bus.name = node_bus_map.at("name"s).AsString();
        const auto& stops = node_bus_map.at("stops").AsArray();

        // маршрут в прямом направлении
        for (const auto& stop : stops) {
            bus.route.push_back(ts.FindStop(stop.AsString()));
        }

        // если маршрут не кольцевой
        if (!bus.is_roundtrip) {
            // Добавляем остановки в обратном направлении
            for (size_t i = stops.size() - 1; i > 1; --i) {
                bus.route.push_back(ts.FindStop(stops[i - 1].AsString()));
            }
            // Добавляем первую остановку
            bus.route.push_back(ts.FindStop(stops[0].AsString()));
        }

        return bus;
    }

    // с помощью вспомогательных методов заполняем транспортный каталог из json
    transport::TransportCatalogue JsonReader::TransportCatalogueFromJson() const {
        transport::TransportCatalogue ts;
        for (const auto& request : json_document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
            if (request.AsDict().at("type"s).AsString() == "Stop"sv) {
                ts.AddStop(ParseStopQuery(request.AsDict()));
            }
        }

        for (const auto& request : json_document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
            if (request.AsDict().at("type"s).AsString() == "Stop"sv) {
                ParseStopQueryDistance(ts, request);
            }
        }

        for (const auto& request : json_document_.GetRoot().AsDict().at("base_requests"s).AsArray()) {
            if (request.AsDict().at("type"s).AsString() == "Bus"sv) {
                ts.AddBus(ParseBusQuery(ts, request));
            }
        }
        return ts;
    }

    renderer::RenderSettings JsonReader::ParseRenderSettings() const {
        renderer::RenderSettings rs;

        const auto rs_map = json_document_.GetRoot().AsDict().at("render_settings"s).AsDict();

        // парсинг из json в структуру RenderSettings
        rs.width = rs_map.at("width"s).AsDouble();
        rs.height = rs_map.at("height"s).AsDouble();

        rs.padding = rs_map.at("padding"s).AsDouble();
        rs.line_width = rs_map.at("line_width"s).AsDouble();;

        rs.stop_radius = rs_map.at("stop_radius"s).AsDouble();
        rs.bus_label_font_size = rs_map.at("bus_label_font_size"s).AsInt();
        rs.bus_label_offset = svg::Point{ rs_map.at("bus_label_offset"s).AsArray().at(0).AsDouble(),
                                          rs_map.at("bus_label_offset"s).AsArray().at(1).AsDouble() };

        rs.stop_label_font_size = rs_map.at("stop_label_font_size"s).AsInt();

        rs.stop_label_offset = svg::Point{ rs_map.at("stop_label_offset"s).AsArray().at(0).AsDouble(),
                                          rs_map.at("stop_label_offset"s).AsArray().at(1).AsDouble() };

        rs.underlayer_color = ParseColor(rs_map.at("underlayer_color"s));
        rs.underlayer_width = rs_map.at("underlayer_width"s).AsDouble();
        const auto& color_pl = rs_map.at("color_palette"s).AsArray();
        rs.color_palette.reserve(color_pl.size());

        for (const auto& color : color_pl) {
            rs.color_palette.emplace_back(ParseColor(color));
        }

        return rs;
    }

    renderer::MapRenderer JsonReader::MapRenderFromJson(const transport::TransportCatalogue& ts) const {
        std::vector<geo::Coordinates> stops_coords;
        stops_coords.reserve(ts.GetStops().size());
        for (const Bus& bus : ts.GetBuses()) {
            for (const Stop* stop : bus.route) {
                stops_coords.emplace_back(stop->coordinates);
            }
        }

        return  renderer::MapRenderer(std::move(ParseRenderSettings()), stops_coords);
    }

    svg::Color JsonReader::ParseColor(const json::Node& node) const {
        if (node.IsString()) {
            return node.AsString();
        }

        const auto& array = node.AsArray();
        uint8_t red = array.at(0).AsInt();
        uint8_t green = array.at(1).AsInt();
        uint8_t blue = array.at(2).AsInt();

        if (array.size() == 3) {
            return svg::Rgb(red, green, blue);
        }

        double alpha = array.at(3).AsDouble();
        return svg::Rgba(red, green, blue, alpha);
    }

    transport::RouterSettings JsonReader::ParseRoutSettings() const {
        transport::RouterSettings settings;

        const auto rs_map = json_document_.GetRoot().AsDict().at("routing_settings"s).AsDict();

        settings.bus_wait_time_ = rs_map.at("bus_wait_time"s).AsInt();
        settings.bus_velocity_ = rs_map.at("bus_velocity"s).AsDouble();

        return settings;
    }

}  // namespace json_reader
