#include <iostream>
#include <fstream>
#include <string>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "json_builder.h"

using namespace std::literals;


int main() {
    
    /*
    //Для чтения из файла

    std::ifstream input_file("input000.json");
    if (!input_file.is_open()) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    std::ofstream output_file("output000.svg");
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }
    */
    
    
    json_reader::JsonReader json_reader(std::cin);

    transport::TransportCatalogue transport_catalogue = json_reader.TransportCatalogueFromJson();

    renderer::MapRenderer map_render = json_reader.MapRenderFromJson(transport_catalogue);

    transport::RouterSettings router_settings = json_reader.ParseRoutSettings();

    transport::Router router = { router_settings, transport_catalogue };

    transport::RequestHandler request_handler(transport_catalogue, map_render, router);

    json_reader.ResponseRequests(std::cout, request_handler);
    

    return 0;
}

