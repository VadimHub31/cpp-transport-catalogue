#include "map_renderer.h"

#include <set>

using namespace std;
using namespace svg;
using namespace geo;

MapRenderer::MapRenderer() {}

MapRenderer& MapRenderer::SetRenderSettings(RenderSettings& render_settings) {
    render_settings_ = move(render_settings);
    return *this;
}

Document MapRenderer::Render(const map<string_view, Bus*>* bus_list) const {
    Document document;
    if (!bus_list) {
        return document;
    }
    
    auto comp = [](const Stop* stop_lhs, const Stop* stop_rhs) {
        return stop_lhs->name < stop_rhs->name;
    };
    
    set<const Stop*, decltype(comp)> stops(comp);
    for (const auto& [bus_name, bus] : *bus_list) {
        for (const Stop* stop : bus->stops) {
            stops.insert(stop);
        }
    }

    vector<Coordinates> all_coords;
    for (const auto& stop : stops) {
        all_coords.push_back(stop->coordinates);
    }
    SphereProjector projector(all_coords.begin(), all_coords.end(), render_settings_.width, render_settings_.height, render_settings_.padding);
    
    size_t color = 0;
    DrawTheRoutes(bus_list, projector, color, document);

    color = 0;
    DrawTheRouteNames(bus_list, projector, color, document);

    for (const Stop* stop : stops) {
        auto stop_point = DrawTheCircle(projector(stop->coordinates));
        document.Add(stop_point);
    }

    for (const Stop* stop : stops) {
        auto stop_underlayer = DrawTheStopUnderlayer(stop, projector(stop->coordinates));
        auto stop_name = DrawTheStopName(stop, projector(stop->coordinates));
        
        document.Add(stop_underlayer);
        document.Add(stop_name);
    }

    return document;
}

void MapRenderer::DrawTheRouteNames(const map<string_view, Bus *> *bus_list, SphereProjector& projector, size_t color, svg::Document& document) const {
    for (const auto& [bus_name, bus] : *bus_list) {
        if (!bus->stops.empty()) {
            Text underlayer_route = DrawTheRouteUnderlayer(bus, projector(bus->stops[0]->coordinates));
            Text route_name = DrawTheRouteName(bus, projector(bus->stops[0]->coordinates), color % (render_settings_.color_palette).size());
            document.Add(underlayer_route);
            document.Add(route_name);
            if (!bus->is_roundtrip && bus->stops[bus->stops.size() / 2]->name != bus->stops[0]->name) {
                Text second_underlayer_route = DrawTheRouteUnderlayer(bus, projector(bus->stops[bus->stops.size() / 2]->coordinates));
                Text second_route_name = DrawTheRouteName(bus, projector(bus->stops[bus->stops.size() / 2]->coordinates), color % (render_settings_.color_palette).size());                    
                document.Add(second_underlayer_route);
                document.Add(second_route_name);
            }

            ++color;
        }
    }
}

void MapRenderer::DrawTheRoutes(const map<string_view, Bus *> *bus_list, SphereProjector& projector, size_t color, svg::Document& document) const {
    for (const auto& [bus_name, bus] : *bus_list) {
        if (!bus->stops.empty()) {
            Polyline route = DrawTheRoute(bus, projector, color % (render_settings_.color_palette).size());
            document.Add(route);
            ++color;
        }
    }
}

Polyline MapRenderer::DrawTheRoute(const Bus* bus, SphereProjector& projector, size_t color) const {
    auto route_grafic = Polyline();
    for (const auto& stop : bus->stops) {
        route_grafic.AddPoint(projector(stop->coordinates));
    }

    route_grafic.SetStrokeColor(render_settings_.color_palette[color])
                .SetFillColor(NoneColor)
                .SetStrokeWidth(render_settings_.line_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
    return route_grafic;
}

Text MapRenderer::DrawTheRouteUnderlayer(const Bus* bus, Point coord) const {
    auto underlayer = Text().SetPosition(coord)
                            .SetStrokeWidth(render_settings_.underlayer_width)
                            .SetOffset(Point{render_settings_.bus_label_offset[0], render_settings_.bus_label_offset[1]})
                            .SetFontSize(render_settings_.bus_label_font_size)
                            .SetFontFamily("Verdana"s)
                            .SetFontWeight("bold"s)
                            .SetData(bus->name)
                            .SetFillColor(render_settings_.underlayer_color)
                            .SetStrokeColor(render_settings_.underlayer_color)
                            .SetStrokeLineCap(StrokeLineCap::ROUND)
                            .SetStrokeLineJoin(StrokeLineJoin::ROUND);
    return underlayer;
}

Text MapRenderer::DrawTheRouteName(const Bus* bus, Point coord, size_t color) const {
    auto name = Text().SetPosition(coord)
                      .SetOffset(Point{render_settings_.bus_label_offset[0], render_settings_.bus_label_offset[1]})
                      .SetFontSize(render_settings_.bus_label_font_size)
                      .SetFontFamily("Verdana"s)
                      .SetFontWeight("bold"s)
                      .SetData(bus->name)
                      .SetFillColor(render_settings_.color_palette[color]);
    return name;
}

Circle MapRenderer::DrawTheCircle(Point coord) const {
    auto circle = Circle().SetFillColor("white"s).SetCenter(coord).SetRadius(render_settings_.stop_radius);
    return circle;
}

Text MapRenderer::DrawTheStopName(const Stop* stop, Point coord) const {
    auto name = Text().SetPosition(coord)
                      .SetOffset(Point{render_settings_.stop_label_offset[0], render_settings_.stop_label_offset[1]})
                      .SetFontSize(render_settings_.stop_label_font_size)
                      .SetFontFamily("Verdana"s)
                      .SetData(stop->name)
                      .SetFillColor("black"s);
    return name;
}

Text MapRenderer::DrawTheStopUnderlayer(const Stop* stop, Point coord) const {
    auto underlayer = Text().SetPosition(coord)
                      .SetOffset(Point{render_settings_.stop_label_offset[0], render_settings_.stop_label_offset[1]})
                      .SetFontSize(render_settings_.stop_label_font_size)
                      .SetFontFamily("Verdana"s)
                      .SetData(stop->name)
                      .SetFillColor(render_settings_.underlayer_color)
                      .SetStrokeColor(render_settings_.underlayer_color)
                      .SetStrokeWidth(render_settings_.underlayer_width)
                      .SetStrokeLineCap(StrokeLineCap::ROUND)
                      .SetStrokeLineJoin(StrokeLineJoin::ROUND);
    return underlayer;
}
