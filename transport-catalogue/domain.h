#pragma once
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"


namespace domain {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<const Stop*> route;
		bool is_roundtrip;
	};

	struct BusInfo {
		std::string_view bus_name;
		size_t stops = 0;
		size_t uniq_stops = 0;
		size_t request_id = 0;  // номер запроса
		double route_length = 0;  // фактическая длина маршрута в метрах
		double curvature = 0;  // извилистость, отношение фактической длины маршрута к географическому расстоянию
	};

	struct StopInfo {
		size_t request_id;  // номер запроса
		std::set<std::string_view> buses;  // список маршрутов
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

	size_t GetUniqStops(std::vector<const Stop*> stops);

}  // namespace domain