#pragma once

#include <deque>
#include <optional>
#include <unordered_map>

#include "geo.h"
#include "domain.h"

using namespace domain;

namespace transport {
	
	class TransportCatalogue {

	public:
		void AddBus(Bus&& bus);

		void AddStop(Stop&& stop);

		void AddStopPairDistances(const Stop* from, const Stop* to, size_t distance);

		[[nodiscard]] std::optional<size_t> GetStopPairDistances(const Stop* from, const Stop* to) const;

		[[nodiscard]] const Bus* FindBus(std::string_view name_bus) const;

		[[nodiscard]] const Stop* FindStop(std::string_view name_stop) const;

		[[nodiscard]] std::optional<BusInfo> GetBusInfo(std::string_view name_bus) const;

		[[nodiscard]] std::optional<StopInfo> GetBusesForStop(std::string_view stop_name) const;

		inline const std::deque<domain::Bus>& GetBuses() const {
			return buses_;
		}

		inline const std::deque<domain::Stop>& GetStops() const {
			return stops_;
		}

		const std::deque<const domain::Stop*> BusesForStop() const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;  // остановки с именами

		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;  // автобусы с именами

		std::unordered_map<std::string_view, std::set<std::string_view>> buses_for_stop_;  // список автобусов на остановке

		std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, StopPairHasher> stop_pair_distances_;  // список расстояний между парой остановок

		static double ComputeGeoRouteLength(const Bus* bus);

		size_t ComputeLinearRouteLength(const Bus* bus) const;
	};

} // namespace transport