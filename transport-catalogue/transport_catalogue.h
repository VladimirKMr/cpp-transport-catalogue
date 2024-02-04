#pragma once

#include <deque>
#include <iomanip>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace transport {

	struct Stop {
		std::string name;
		Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<const Stop*> route;
	};

	struct BusInfo {
		std::string_view bus_name;
		size_t stops = 0;
		size_t uniq_stops = 0;
		double route_length = 0;
	};

	struct StopInfo {
		std::set<std::string_view> buses;
	};

	struct StopPairHasher {
	private:
		std::hash<const void*> hasher_;

	public:
		uint64_t operator()(const std::pair<const Stop*, const Stop*>& stop_pair) const {
			uint64_t first_hash = hasher_(stop_pair.first);
			uint64_t second_hash = hasher_(stop_pair.second);

			return first_hash * 37 + second_hash * (38 * 38);
		}
	};
	
	class TransportCatalogue {

	public:
		void AddBus(Bus&& bus);

		void AddStop(Stop&& stop);

		[[nodiscard]] const Bus* FindBus(std::string_view name_bus) const;

		[[nodiscard]] const Stop* FindStop(std::string_view name_stop) const;

		[[nodiscard]] std::optional<BusInfo> GetBusInfo(std::string_view name_bus) const;

		[[nodiscard]] std::optional<StopInfo> GetBusesForStop(std::string_view stop_name) const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
		std::unordered_map<std::string_view, std::set<std::string_view>> buses_for_stop_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, StopPairHasher> distances_pair_stops_; // пока не нужен

		static double ComputeRouteLength(const Bus* bus);

		static size_t GetUniqStops(std::vector<const Stop*> stops);
	};

} // namespace transport