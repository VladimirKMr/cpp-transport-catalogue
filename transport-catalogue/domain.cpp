#include <algorithm>
#include "domain.h"

namespace domain {

	size_t GetUniqStops(std::vector<const Stop*> stops) {
		
		std::sort(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name < rhs->name; });
		
		auto last = std::unique(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name == rhs->name; });
		return static_cast<size_t>(std::distance(stops.begin(), last));
	}

}