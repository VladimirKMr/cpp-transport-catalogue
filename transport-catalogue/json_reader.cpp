#include "json_reader.h"
#include "transport_catalogue.h"
#include "geo.h"
#include "json.h"
#include "json_builder.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace json_reader {

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
        }
        responses.EndArray();
        json::Print(json::Document(responses.Build()), os);
    }

    Stop JsonReader::ParseStopQuery(const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsDict();
        return Stop{ type_stop_map.at("name"s).AsString(),
                        geo::Coordinates{type_stop_map.at("latitude"s).AsDouble(),
                                         type_stop_map.at("longitude"s).AsDouble()} };
    }

    void JsonReader::ParseStopQueryDistance(transport::TransportCatalogue& ts, const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsDict();
        for (const auto& [to_stop, dist_to_stop] : type_stop_map.at("road_distances"s).AsDict()) {
            ts.AddStopPairDistances(ts.FindStop(type_stop_map.at("name"s).AsString()),
                                    ts.FindStop(to_stop), dist_to_stop.AsInt());
        }

    }

    Bus JsonReader::ParseBusQuery(const transport::TransportCatalogue& ts, const json::Node& node_bus) const {
        const auto& node_bus_map = node_bus.AsDict();
        Bus bus;
        bus.is_roundtrip = node_bus_map.at("is_roundtrip"s).AsBool();
        bus.name = node_bus_map.at("name"s).AsString();
        const auto& stops = node_bus_map.at("stops").AsArray();

        for (const auto& stop : stops) {
            bus.route.push_back(ts.FindStop(stop.AsString()));
        }

        if (!bus.is_roundtrip) {
            
            for (size_t i = stops.size() - 1; i > 1; --i) {
                bus.route.push_back(ts.FindStop(stops[i - 1].AsString()));
            }
            bus.route.push_back(ts.FindStop(stops[0].AsString()));
        }

        return bus;
    }

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

}  // namespace json_reader
