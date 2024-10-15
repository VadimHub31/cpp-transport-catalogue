#pragma once

#include "geo.h"

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace transport_catalogue {

struct Stop {
    std::string name;
    Coordinates coordinates;
};
struct Bus {
    std::string name;
    std::vector<Stop*> stops;
};
struct BusInfo {
    size_t stops;
    size_t unique_stops;
    double route_length;
};

class TransportCatalogue {
public:

    void AddBus(const Bus& bus_to_add);
    void AddStop(const Stop& stop_to_add);
    const Bus* FindBus(std::string_view busname) const;
    const Stop* FindStop(std::string_view stopname) const;
    const BusInfo GetBusInfo(const Bus& bus) const;
    std::vector<Bus*> GetStopInfo(const Stop& stop) const;

private:
    std::unordered_map<std::string_view, Bus*> busname_to_bus_;
    std::deque<Bus> buses_;

    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::deque<Stop> stops_;

    std::unordered_map<Stop*, std::vector<Bus*>> stop_to_buses_;
};

inline size_t GetStopsNum(const Bus& bus) {
    return bus.stops.size();
}

inline size_t GetUniqueStopsNum(const Bus& bus) {
    std::unordered_set<Stop*> unique_stops;
    for (const auto stop : bus.stops) {
        unique_stops.insert(stop);
    }
    return unique_stops.size();
}

inline double GetRouteLength(const Bus& bus) {
    double result = 0.0;

    for (size_t i = 1; i != bus.stops.size(); ++i) {
        if (bus.stops[i - 1] && bus.stops[i]) {
            result += ComputeDistance(bus.stops[i - 1]->coordinates, bus.stops[i]->coordinates);
        }
    }
    return result;
}

}
