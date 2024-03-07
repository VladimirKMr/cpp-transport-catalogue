#include "json_reader.h"
#include "transport_catalogue.h"
#include "geo.h"
#include "json.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace json_reader {

    // ��������������� ����� ��� ������ ��������� �� ������� Stop, � ������� json
    json::Dict JsonReader::StopResponseToJsonDict(int request_id, const std::optional<domain::StopInfo>& stop_info) const {
        json::Dict result;
        if (stop_info) {
            json::Array buses_array;
            for (const auto& bus : stop_info->buses) {
                buses_array.push_back(json::Node(std::string(bus)));
            }
            result["buses"] = std::move(buses_array);
        }
        else {
            result["error_message"] = json::Node(std::string("not found"));
        }
        result["request_id"] = request_id;
        return result;
    }

    // ��������������� ����� ��� ������ ���������� � �������� �� ������� Bus, � ������� json
    json::Dict JsonReader::BusResponseToJsonDict(int request_id, const std::optional<domain::BusInfo>& bus_info) const {
        json::Dict result;
        if (bus_info) {
            result["curvature"] = bus_info->curvature;
            result["route_length"] = bus_info->route_length;
            result["stop_count"] = (int)bus_info->stops;
            result["unique_stop_count"] = (int)bus_info->uniq_stops;
        }
        else {
            result["error_message"] = json::Node(std::string("not found"));
        }
        result["request_id"] = request_id;
        return result;
    }

    // ��������������� ����� ��� ������ svg ������� �� ������� Map
    json::Dict JsonReader::MapResponseToJsonDict(int request_id, const svg::Document& render_doc) const {
        json::Dict result;

        std::ostringstream out;
        render_doc.Render(out);
        std::string svg_out = out.str();

        result["map"] = svg_out;
        result["request_id"] = request_id;

        return result;
    }

    // ��������� �������� � �������� � ����� ���������� � ������� ��������������� ������� ����������� � json
    void JsonReader::ResponseRequests(std::ostream& os, const transport::RequestHandler& rq) const {
        json::Array responses;

        for (const auto& request : json_document_.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
            const auto& request_id = request.AsMap().at("id"s).AsInt();
            if (request.AsMap().at("type"s).AsString() == "Stop"sv) {
                auto result = rq.GetBusesByStop(request.AsMap().at("name"s).AsString());
                responses.push_back(StopResponseToJsonDict(request_id, result));
            }
            else if (request.AsMap().at("type"s).AsString() == "Bus"sv) {
                auto result = rq.GetBusInfo(request.AsMap().at("name"s).AsString());
                responses.push_back(BusResponseToJsonDict(request_id, result));
            }
            else if (request.AsMap().at("type"s).AsString() == "Map"sv) {
                responses.push_back(MapResponseToJsonDict(request_id, rq.RenderMap()));
            }
        }

        json::Document result{ responses };
        json::Print(result, os);
    }

    // ��������������� ����� ��� ���������� ��������, �������� ���������� �� ���������
    Stop JsonReader::ParseStopQuery(const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsMap();
        return Stop{ type_stop_map.at("name"s).AsString(),
                        geo::Coordinates{type_stop_map.at("latitude"s).AsDouble(),
                                         type_stop_map.at("longitude"s).AsDouble()} };
    }

    // ��������������� ����� ��� ���������� ��������, �������� ���������� � ���������� ����� �����������
    void JsonReader::ParseStopQueryDistance(transport::TransportCatalogue& ts, const json::Node& type_stop) const {
        const auto& type_stop_map = type_stop.AsMap();
        for (const auto& [to_stop, dist_to_stop] : type_stop_map.at("road_distances"s).AsMap()) {
            ts.AddStopPairDistances(ts.FindStop(type_stop_map.at("name"s).AsString()),
                                    ts.FindStop(to_stop), dist_to_stop.AsInt());
        }

    }

    // ��������������� ����� ��� ���������� ��������, ��������� ��������� � ������� � ������������ � ������ is_roundtrip (��������� ��� ���)
    Bus JsonReader::ParseBusQuery(const transport::TransportCatalogue& ts, const json::Node& node_bus) const {
        const auto& node_bus_map = node_bus.AsMap();
        Bus bus;
        bus.is_roundtrip = node_bus_map.at("is_roundtrip"s).AsBool();
        bus.name = node_bus_map.at("name"s).AsString();
        const auto& stops = node_bus_map.at("stops").AsArray();

        // ������� � ������ �����������
        for (const auto& stop : stops) {
            bus.route.push_back(ts.FindStop(stop.AsString()));
        }

        // ���� ������� �� ���������
        if (!bus.is_roundtrip) {
            // ��������� ��������� � �������� �����������
            for (size_t i = stops.size() - 1; i > 1; --i) {
                bus.route.push_back(ts.FindStop(stops[i - 1].AsString()));
            }
            // ��������� ������ ���������
            bus.route.push_back(ts.FindStop(stops[0].AsString()));
        }

        return bus;
    }

    // � ������� ��������������� ������� ��������� ������������ ������� �� json
    transport::TransportCatalogue JsonReader::TransportCatalogueFromJson() const {
        transport::TransportCatalogue ts;
        for (const auto& request : json_document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
            if (request.AsMap().at("type"s).AsString() == "Stop"sv) {
                ts.AddStop(ParseStopQuery(request.AsMap()));
            }
        }

        for (const auto& request : json_document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
            if (request.AsMap().at("type"s).AsString() == "Stop"sv) {
                ParseStopQueryDistance(ts, request);
            }
        }

        for (const auto& request : json_document_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
            if (request.AsMap().at("type"s).AsString() == "Bus"sv) {
                ts.AddBus(ParseBusQuery(ts, request));
            }
        }
        return ts;
    }

    renderer::RenderSettings JsonReader::ParseRenderSettings() const {
        renderer::RenderSettings rs;

        const auto rs_map = json_document_.GetRoot().AsMap().at("render_settings"s).AsMap();

        // ������� �� json � ��������� RenderSettings
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
