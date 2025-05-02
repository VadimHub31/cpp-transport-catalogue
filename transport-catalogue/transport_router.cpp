#include "transport_router.h"

using namespace std;
using namespace transport_catalogue;
using namespace graph;

TransportRouter::TransportRouter(const TransportCatalogue& transport_catalogue) : transport_catalogue_(move(transport_catalogue)) {}

void TransportRouter::SetSettings(double wait_time, double avg_bus_speed) {
    wait_time_ = wait_time;
    avg_bus_speed_ = avg_bus_speed;

    BuildRoute();
}

const pair<VertexId, VertexId>& TransportRouter::GetVerticesByStop(const Stop* stop) const {
    return stop_to_vertices_.at(stop);
}

std::optional<RouteItem> TransportRouter::GetRouteStat(const Stop* from, const Stop* to) const {
    RouteItem route_items;
    
    auto stop_from = transport_catalogue_.FindStop(from->name);
    auto stop_to = transport_catalogue_.FindStop(to->name);

    auto vertex_from = GetVerticesByStop(stop_from);
    auto vertex_to = GetVerticesByStop(stop_to);

    auto route_info = router_->BuildRoute(vertex_from.first, vertex_to.first);

    if (route_info.has_value()) {
        route_items.all_time = route_info.value().weight;
        for (const auto& edge : route_info.value().edges) {
            route_items.items.push_back(edgeId_to_item_.at(edge));
        }
        return route_items;
    }

    return nullopt;
}

void TransportRouter::AddStopsInGraph() {
    size_t i = 0;
    auto stops = transport_catalogue_.GetStops();

    for (const Stop& stop : stops) {
        auto found_stop = transport_catalogue_.FindStop(stop.name);
        stop_to_vertices_.insert({found_stop, {i, i + 1}});

        EdgeId edge = dw_graph_->AddEdge({i, i + 1, wait_time_});
        Item item({"Wait"s, stop.name, wait_time_, 1});
        edgeId_to_item_.insert({edge, item});

        i += 2;
    }
}

void TransportRouter::AddBusEdgeinGraph(const Stop* from, const Stop* to, const std::string& bus_name, int span_count, double distance) {
    Item item({"Bus"s, bus_name, (distance * 60) / (avg_bus_speed_ * 1000), span_count});

    auto from_vertex = GetVerticesByStop(from);
    auto to_vertex = GetVerticesByStop(to);

    EdgeId edge = dw_graph_->AddEdge({from_vertex.second, to_vertex.first, (distance * 60) / (avg_bus_speed_ * 1000)});
    edgeId_to_item_.insert({edge, item});
}

void TransportRouter::AddBusInGraph(const Bus* bus) {
    for (size_t i = 0; i < (bus->stops.size() - 1); ++i) {
        double from_dist = 0.0;
        double to_dist = 0.0;

        const Stop* fromi = bus->stops[i];

        for (size_t j = i; j < (bus->stops.size() - 1); ++j) {
            const Stop* from = bus->stops[j];
            const Stop* to = bus->stops[j + 1];
            
            from_dist += transport_catalogue_.GetStopsDistance(from, to);
            AddBusEdgeinGraph(fromi, to, bus->name, j + 1 - i, from_dist);

            if (!bus->is_roundtrip) {
                to_dist += transport_catalogue_.GetStopsDistance(to, from);
                AddBusEdgeinGraph(to, fromi, bus->name, j + 1 - i, to_dist);
            }
        }
    }
}

void TransportRouter::BuildRoute() {
    dw_graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(
        transport_catalogue_.GetStops().size() * 2
    );
    AddStopsInGraph();

    auto buses = transport_catalogue_.GetBuses();
    for (const auto& [_, bus] : *buses) {
        AddBusInGraph(bus);
    }

    router_ = std::make_unique<graph::Router<double>>(*dw_graph_);
}

