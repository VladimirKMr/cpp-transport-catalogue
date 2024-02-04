#include <algorithm>
#include <string_view>
#include <vector>

#include "transport_catalogue.h"

namespace transport {

    void TransportCatalogue::AddBus(Bus&& bus) {
		buses_.push_back(std::move(bus));
		busname_to_bus_[buses_.back().name] = &buses_.back();
		for (const Stop* stop : buses_.back().route) {
        	buses_for_stop_[stop->name].emplace(buses_.back().name);
    	}
	}

    void TransportCatalogue::AddStop(Stop&& stop) {
		stops_.push_back(std::move(stop));
		stopname_to_stop_[stops_.back().name] = &stops_.back();
		buses_for_stop_[stops_.back().name];
	}

    const Bus* TransportCatalogue::FindBus(std::string_view name_bus) const {
		if (busname_to_bus_.count(name_bus)) {
			return busname_to_bus_.at(name_bus);
		}
		return nullptr;
	}

    const Stop* TransportCatalogue::FindStop(std::string_view name_stop) const {
        if (stopname_to_stop_.count(name_stop)) {
            return stopname_to_stop_.at(name_stop);
        }
        return nullptr;
    }

	double TransportCatalogue::ComputeRouteLength(const Bus* bus) {

		if (!bus) {
			return 0;
		}

		double route_length = 0;

		const auto& route = bus->route;

		for (size_t i = 1; i < route.size(); ++i) {
			const auto& from_stop = route[i - 1];
			const auto& to_stop = route[i];
			
			double distance = ComputeDistance(from_stop->coordinates, to_stop->coordinates);
			
			route_length += distance;
		}

		return route_length;
	}

    size_t TransportCatalogue::GetUniqStops(std::vector<const Stop*> stops) {
        std::sort(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name < rhs->name;});
		auto last = std::unique(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name == rhs->name; });
		return static_cast<size_t>(std::distance(stops.begin(),last));
    }    

    std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name_bus) const {

		auto it = busname_to_bus_.find(name_bus);

		if (it != busname_to_bus_.end()) {
			const Bus& bus = *(it->second);
			BusInfo bus_info;
			bus_info.bus_name = bus.name;
			bus_info.stops = bus.route.size();
			bus_info.uniq_stops = GetUniqStops(bus.route);
			bus_info.route_length = ComputeRouteLength(&bus);
			return bus_info;
		} else {
			return std::nullopt;
		}
    }

	std::optional<StopInfo> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {

		auto it = buses_for_stop_.find(stop_name);

		if (it != buses_for_stop_.end()) {
			StopInfo stop_info;
			stop_info.buses = it->second;
			return stop_info;
		} else {
			return std::nullopt;
		}
	}

} // namespace transport