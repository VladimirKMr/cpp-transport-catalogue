#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace transport {
    

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find_first_of(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    auto comma2 = str.find_first_of(',', not_space2);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}


/*
  Парсит команду в структуру CommandDescription
*/
CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

/*
  Парсит дистанции до остановки Stop от остановок #stop, для заполнения индекса stop_pair_distances_
*/
void InputReader::ParseStopDistances(const CommandDescription& command, TransportCatalogue& catalogue) const {
    std::istringstream iss(command.description);

    // Пропускаем координаты
    std::string coord;
    for (int i = 0; i < 2; ++i) {
        std::getline(iss, coord, ',');
    }

    // Начинаем парсить расстояния
    std::string distances_part;
    while (std::getline(iss, distances_part, ',')) {
        std::istringstream distances_stream(distances_part);
        int distance;
        distances_stream >> distance;

        // Пропускаем m и to (между расстоянием и названием остановки)
        distances_stream.ignore(std::numeric_limits<std::streamsize>::max(), 'm');
        distances_stream.ignore(std::numeric_limits<std::streamsize>::max(), 't');
        distances_stream.ignore(std::numeric_limits<std::streamsize>::max(), 'o');

        // Добавляем имя остановки (до запятой)
        std::string stop_name;
        std::getline(distances_stream >> std::ws, stop_name, ',');

        // Удаляем пробелы
        stop_name.erase(stop_name.find_last_not_of(" \n\r\t") + 1);

        // Заполняем контейнер
        catalogue.AddStopPairDistances(std::move(catalogue.FindStop(command.id)), std::move(catalogue.FindStop(stop_name)), static_cast<size_t>(distance));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {

    // Первый проход - добавление остановок с координатами
    for (auto& command : commands_) {
        if (command.command == "Stop") {
            Stop stop;
            stop.name = command.id;
            stop.coordinates = ParseCoordinates(command.description);
            catalogue.AddStop(std::move(stop));
        }
        
    }

    // Второй проход - добавление расстояний между парами остановок
    for (auto& command : commands_) {
        if (command.command == "Stop") {
            ParseStopDistances(command, catalogue);
        }
    }

    // Добавление автобуса
    for (auto& command : commands_) {
        if (command.command == "Bus") {
            Bus bus;
            bus.name = command.id;
            auto stops = ParseRoute(command.description);

            for (const auto& stop : stops) {
                const Stop* stop_ptr = catalogue.FindStop(stop);
                if (stop_ptr) {
                    bus.route.push_back(stop_ptr);
                }
            }

            if (!bus.route.empty()) {
                catalogue.AddBus(std::move(bus));
            }
        }
    }

}

void InputReader::ReadRequestToAdd(std::istream& input) {
    int base_request_count;
    input >> base_request_count >> std::ws;
    for (int i = 0; i < base_request_count; ++i) {
        std::string line;
        getline(input, line);
        this->ParseLine(line);
    }
}

} // namespace transport