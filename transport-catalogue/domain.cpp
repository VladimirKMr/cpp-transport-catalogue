#include <algorithm>
#include "domain.h"

namespace domain {

	// Функция определения уникальных остановок
	size_t GetUniqStops(std::vector<const Stop*> stops) {
		// сортируем все остановки по наименованию
		std::sort(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name < rhs->name; });
		// исключаем повторы имен остановок
		auto last = std::unique(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) { return lhs->name == rhs->name; });
		return static_cast<size_t>(std::distance(stops.begin(), last));
	}

}