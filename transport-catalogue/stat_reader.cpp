#include "stat_reader.h"
#include "input_reader.h"

namespace transport {

void PrintBusInfo(const std::optional<BusInfo>& bus_info, std::string_view request_id, std::ostream& output) {

    if (bus_info.has_value()) {
        BusInfo info = *bus_info; 
        output << "Bus " << info.bus_name << ": " << info.stops << " stops on route, " 
            << info.uniq_stops << " unique stops, " << info.route_length << " route length" << std::endl;
    } else {
        output << "Bus " << request_id << ": not found" << std::endl;
    }
}

void PrintStopInfo(const std::optional<StopInfo>& stop_info, std::string_view request_id, std::ostream& output) {

    if (stop_info.has_value()) {
        const auto& buses = stop_info.value().buses;
        if (!buses.empty()) {
            output << "Stop " << request_id << ": buses";
            for (const auto& bus : buses) {
                output << " " << bus;
            }
            output << std::endl;
        } else {
            output << "Stop " << request_id << ": no buses" << std::endl;
        }
    } else {
        output << "Stop " << request_id << ": not found" << std::endl;
    }
}

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {

    auto command_start = request.find_first_not_of(' ');
    auto command_end = request.find_last_not_of(' ');

    if (command_start == request.npos) {
        output << "Error. Incorrect request." << std::endl;
        return;
    }

    auto space_pos = request.find(' ', command_start);
    if (space_pos == std::string_view::npos) {
        space_pos = command_end;
    }

    auto command = request.substr(command_start, space_pos - command_start);
    auto request_id = request.substr(space_pos + 1, command_end - space_pos);

    if (command == "Bus") {
        PrintBusInfo(transport_catalogue.GetBusInfo(request_id), request_id, output);
    } else if (command == "Stop") {
        PrintStopInfo(transport_catalogue.GetBusesForStop(request_id), request_id, output);
    } else {
        output << "Error. Unknown command." << std::endl;
    }
}

} // namespace transport

