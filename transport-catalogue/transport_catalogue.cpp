#include "transport_catalogue.h"

#include <algorithm>

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddBus(const Bus& bus_to_add) {
    buses_.push_back(bus_to_add);
    busname_to_bus_[buses_.back().name] = &buses_.back();

    for (const Stop* stop : bus_to_add.stops) {
        if (!count(stop_to_buses_.at(stop).begin(), stop_to_buses_.at(stop).end(), busname_to_bus_[buses_.back().name])) {
            stop_to_buses_[stop].push_back(busname_to_bus_[buses_.back().name]);
        }
    }
}

void TransportCatalogue::AddStop(const Stop& stop_to_add) {
    stops_.push_back(stop_to_add);
    stopname_to_stop_[stops_.back().name] = &stops_.back();

    Stop* stop = stopname_to_stop_[stops_.back().name];
    stop_to_buses_[stop] = {};
}

const Bus* TransportCatalogue::FindBus(string_view busname) const {
    auto it = busname_to_bus_.find(busname);
    if (it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const Stop* TransportCatalogue::FindStop(string_view stopname) const {
    auto it = stopname_to_stop_.find(stopname);
    if (it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::SetStopsDistance(const Stop* from, const Stop* to, int distance) {
    stops_to_distance_[{from, to}] = distance;
}

int TransportCatalogue::GetStopsDistance(const Stop* from, const Stop* to) const {
    auto it = stops_to_distance_.find({from, to});
    if (it != stops_to_distance_.end()) {
        return it->second;
    }
    return GetStopsDistance(to, from);
}

BusInfo TransportCatalogue::GetBusInfo(const Bus& bus) const {
    return {GetStopsNum(bus), GetUniqueStopsNum(bus), GetRouteLength(bus), GetCurvature(bus)};
}

std::vector<const Bus*> TransportCatalogue::GetStopInfo(const Stop& stop) const {
    return stop_to_buses_.at(stopname_to_stop_.at(stop.name));
}

const map<string_view, Bus*>* TransportCatalogue::GetBuses() const {
    auto buses_list_ptr = &busname_to_bus_;
    return buses_list_ptr;
}

}