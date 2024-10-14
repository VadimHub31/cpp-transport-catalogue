#include "transport_catalogue.h"

#include <algorithm>

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddBus(const Bus&& bus_to_add) {
    buses_.push_back(move(bus_to_add));
    busname_to_bus_[buses_.back().name] = &buses_.back();

    for (Stop* stop : bus_to_add.stops) {
        if (!count(stop_to_buses_.at(stop).begin(), stop_to_buses_.at(stop).end(), const_cast<Bus*>(FindBus(move(bus_to_add).name)))) {
            stop_to_buses_[stop].push_back(const_cast<Bus*>(FindBus(move(bus_to_add).name)));
        }
    }
}

void TransportCatalogue::AddStop(const Stop&& stop_to_add) {
    stops_.push_back(move(stop_to_add));
    stopname_to_stop_[stops_.back().name] = &stops_.back();

    Stop* stop = const_cast<Stop*>(FindStop(move(stop_to_add.name)));
    stop_to_buses_[stop] = {};
}

const TransportCatalogue::Bus* TransportCatalogue::FindBus(string_view busname) const {
    if (busname_to_bus_.count(busname)) {
        return busname_to_bus_.at(busname);
    }
    return nullptr;
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(string_view stopname) const {
    if (stopname_to_stop_.count(stopname)) {
        return stopname_to_stop_.at(stopname);
    }    
    return nullptr;
}

const TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(const Bus& bus) const {
    return {GetStopsNum(bus), GetUniqueStopsNum(bus), GetRouteLength(bus)};
}

const std::vector<TransportCatalogue::Bus*> TransportCatalogue::GetStopInfo(const Stop& stop) const {
    return stop_to_buses_.at(const_cast<Stop*>(FindStop(stop.name)));
}

}
