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
    transport::RequestHandler request_handler(transport_catalogue, map_render);
    json_reader.ResponseRequests(std::cout, request_handler);
    

        /*
        json::Print(
            json::Document{
                json::Builder{}
                .StartDict()
                    .Key("key1"s).Value(123)
                    .Key("key2"s).Value("value2"s)
                    .Key("key3"s).StartArray()
                        .Value(456)
                        .StartDict().EndDict()
                        .StartDict()
                            .Key(""s)
                            .Value(nullptr)
                        .EndDict()
                        .Value(""s)
                    .EndArray()
                .EndDict()
                .Build()
            },
            std::cout
        );
        std::cout << std::endl;
        
        json::Print(
            json::Document{
                json::Builder{}
                .Value("just a string"s)
                .Build()
            },
            std::cout
        );

        
        std::cout << std::endl;
        */
    

    //system("pause");

    return 0;
}

