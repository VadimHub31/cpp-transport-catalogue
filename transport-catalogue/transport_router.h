#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

struct Item {
    std::string type, name;
    double time;
    int span_count;
};
struct RouteItem {
    double all_time;
    std::vector<Item> items;
};

class TransportRouter {
public:

    TransportRouter(const transport_catalogue::TransportCatalogue& transport_catalogue);

    void SetSettings(double wait_time, double avg_bus_speed);

    std::optional<RouteItem> GetRouteStat(const Stop* from, const Stop* to) const;

private:
    double wait_time_;
    double avg_bus_speed_;
    const transport_catalogue::TransportCatalogue& transport_catalogue_;
    std::unique_ptr<graph::DirectedWeightedGraph<double>> dw_graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<const Stop*, std::pair<graph::VertexId, graph::VertexId>> stop_to_vertices_;
    std::unordered_map<graph::EdgeId, Item> edgeId_to_item_;
    
    void AddStopsInGraph();
    void AddBusInGraph(const Bus* bus);
    void AddBusEdgeinGraph(const Stop* from, const Stop* to, const std::string& bus_name, int span_count, double distance);
    const std::pair<graph::VertexId, graph::VertexId>& GetVerticesByStop(const Stop* stop) const;

    void BuildRoute();
};