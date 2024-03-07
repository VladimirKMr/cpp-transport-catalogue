#include <algorithm>
#include <string_view>

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

	double TransportCatalogue::ComputeGeoRouteLength(const Bus* bus) {

		if (!bus) {
			return 0;
		}

		double route_length = 0;
		const auto& route = bus->route;

		for (size_t i = 1; i < route.size(); ++i) {
			const auto& from_stop = route[i - 1];
			const auto& to_stop = route[i];
			
			double distance = geo::ComputeDistance(from_stop->coordinates, to_stop->coordinates);
			
			route_length += distance;
		}

		return route_length;
	}

	size_t TransportCatalogue::ComputeLinearRouteLength(const Bus* bus) const {

		if (!bus) {
			return 0;
		}

		size_t route_length = 0;
		const auto& route = bus->route;

		for (size_t i = 1; i < route.size(); ++i) {
			const auto& from_stop = route[i - 1];
			const auto& to_stop = route[i];
			
			size_t distance = 0;

			if (GetStopPairDistances(from_stop, to_stop).has_value()) {
				distance = GetStopPairDistances(from_stop, to_stop).value();
			}
			
			route_length += distance;
		}

		return route_length;
	}

	void TransportCatalogue::AddStopPairDistances(const Stop* from, const Stop* to, size_t distance) {
		stop_pair_distances_[std::make_pair(from,to)] = distance;
	}

	std::optional<size_t> TransportCatalogue::GetStopPairDistances(const Stop* from, const Stop* to) const {
		auto pair_stops = std::make_pair(from, to);

		if (stop_pair_distances_.count(pair_stops)) {
			return stop_pair_distances_.at(pair_stops);
		}

		std::swap(pair_stops.first, pair_stops.second);

		if (!stop_pair_distances_.count(pair_stops)) {
			return std::nullopt;
		}

		return stop_pair_distances_.at(pair_stops);
	}

	std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name_bus) const {

		auto it = busname_to_bus_.find(name_bus);

		if (it != busname_to_bus_.end()) {
			const Bus& bus = *(it->second);
			BusInfo bus_info;
			bus_info.bus_name = bus.name;
			bus_info.stops = bus.route.size();
			bus_info.uniq_stops = GetUniqStops(bus.route);
			bus_info.route_length = static_cast<double>(ComputeLinearRouteLength(&bus));
			bus_info.curvature = bus_info.route_length / ComputeGeoRouteLength(&bus);
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

	const std::deque<const Stop*> TransportCatalogue::BusesForStop() const {
		std::deque<const Stop*> stops_with_buses;
		
		for (const auto& [stop_name, bus_set] : buses_for_stop_) {
			if (!bus_set.empty()) {
				stops_with_buses.push_back(FindStop(stop_name));
			}
		}
		std::sort(stops_with_buses.begin(), stops_with_buses.end(), [](const domain::Stop* lhs, const domain::Stop* rhs) {
			return lhs->name < rhs->name;
			});
		return stops_with_buses;
	}

} // namespace transport 