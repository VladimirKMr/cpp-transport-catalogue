#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace transport {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

class InputReader {
public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(TransportCatalogue& catalogue) const;

    /*
       Парсит запрос с расстояниями, для добавления расстояний между остановками 
    */
    void ParseStopDistances(const CommandDescription& command, TransportCatalogue& catalogue) const;

    // Для чтения запроса из созданного потока
    void ReadRequestToAdd(std::istream& input);

private:
    std::vector<CommandDescription> commands_;
};

} // namespace transport