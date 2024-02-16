#pragma once

#include <iosfwd>
#include <iostream>
#include <string_view>

#include "transport_catalogue.h"

namespace transport {

void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, std::string_view request,
                       std::ostream& output);

// Для чтения запроса из созданного потока
void ReadRequestToInfo(std::istream& input, const TransportCatalogue& catalogue, std::ostream& output);


} // namespace transport