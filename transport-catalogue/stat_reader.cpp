#include "stat_reader.h"

#include <iomanip>
#include <set>

namespace transport_catalogue {

using namespace std::literals;

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    const auto start = request.find_first_not_of(' ');

    request = request.substr(start, request.find_last_not_of(' ') + 1 - start);
    auto request_name = request.substr(0, request.find_first_of(' '));
    auto request_id = request.substr(request.find_first_of(' ') + 1);
    
    output << request_name << " "s << request_id << ": "s;
    
    if (request_name == "Bus"s) {
        auto request_bus = transport_catalogue.FindBus(request_id);
        if (request_bus == nullptr) {
            output << "not found\n"s;
            return;
        }
        const auto bus_info = transport_catalogue.GetBusInfo(*request_bus);
        output << std::setprecision(5) << std::to_string(bus_info.stops) << " stops on route, "s 
               << std::to_string(bus_info.unique_stops) << " unique stops, "s 
               << std::to_string(bus_info.route_length) << " route length, "s 
               << std::to_string(bus_info.curvature) << " curvature\n"s;
    }
    else {
        auto request_stop = transport_catalogue.FindStop(request_id);
        if (request_stop == nullptr) {
            output << "not found\n"s;
            return;
        }
        const auto stop_info = transport_catalogue.GetStopInfo(*request_stop);
        if (stop_info.size() == 0) {
            output << "no buses\n"s;
            return;
        }
        else {
            output << "buses"s;

            std::set<std::string> sorted_info;
            for (const Bus* bus : stop_info) {
                sorted_info.insert(bus->name);
            }
            for (const auto& bus : sorted_info) {
                output << " "s << bus;
            }
            output << std::endl;
        }
    }
}

}
