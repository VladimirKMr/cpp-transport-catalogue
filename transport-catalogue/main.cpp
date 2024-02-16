#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;


int main() {

    transport::TransportCatalogue catalogue;

    {
        transport::InputReader reader;
        
        reader.ReadRequestToAdd(cin);

        reader.ApplyCommands(catalogue);
    }

    {
        ReadRequestToInfo(cin, catalogue, cout);
    }


    return 0;
}

