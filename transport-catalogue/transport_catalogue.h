#pragma once

#include "domain.h"

#include <deque>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace transport_catalogue {

struct StopsHasher {
    std::size_t operator() (std::pair<const Stop*, const Stop*> stops) const {
        std::size_t first_stop_hash = hasher_(static_cast<const void*>(stops.first));
        std::size_t second_stop_hash = hasher_(static_cast<const void*>(stops.second));

        return first_stop_hash + second_stop_hash;
    }
private:
    std::hash<const void*> hasher_;
};

class TransportCatalogue {
public:

    void AddBus(const Bus& bus_to_add);
    void AddStop(const Stop& stop_to_add);
    const Bus* FindBus(std::string_view busname) const;
    const Stop* FindStop(std::string_view stopname) const;
    
    void SetStopsDistance(const Stop* from, const Stop* to, int distance);
    int GetStopsDistance(const Stop* from, const Stop* to) const;

    BusInfo GetBusInfo(const Bus& bus) const;
    std::vector<const Bus*> GetStopInfo(const Stop& stop) const;

    const std::map<std::string_view, Bus*>* GetBuses() const;

private:
    std::map<std::string_view, Bus*> busname_to_bus_;
    std::deque<Bus> buses_;

    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::deque<Stop> stops_;

    std::unordered_map<const Stop*, std::vector<const Bus*>> stop_to_buses_;

    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopsHasher> stops_to_distance_;

    size_t GetStopsNum(const Bus& bus) const {
        return bus.stops.size();
    }

    size_t GetUniqueStopsNum(const Bus& bus) const {
        std::unordered_set<const Stop*> unique_stops;
        for (const auto stop : bus.stops) {
            unique_stops.insert(stop);
        }
        return unique_stops.size();
    }

    int GetRouteLength(const Bus& bus) const {
        int result = 0;
        for (size_t i = 1; i != bus.stops.size(); ++i) {
            result += GetStopsDistance(bus.stops[i - 1], bus.stops[i]);
        }
        return result;
    }

    double GetCurvature(const Bus& bus) const {
        double geo_dist = 0.0;

        for (size_t i = 1; i != bus.stops.size(); ++i) {
            if (bus.stops[i - 1] && bus.stops[i]) {
                geo_dist += ComputeDistance(bus.stops[i - 1]->coordinates, bus.stops[i]->coordinates);
            }
        }
        return GetRouteLength(bus) * 1.0 / geo_dist;
    }
};

}