#include "json_reader.h"
#include "json.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"

#include <algorithm>
#include <sstream>

using namespace std;
using namespace json;
using namespace transport_catalogue;
/*
* Здесь можно разместить код наполнения транспортного справочника данными из JSON,
* а также код обработки запросов к базе и формирование массива ответов в формате JSON
*/

std::string GetColor(const json::Node& value) {
    std::stringstream res;
    if (value.IsString()) {
        res << value.AsString();
    } else {
        const auto& color = value.AsArray();
        if (color.size() == 3) {
            res << "rgb("
                << color[0].AsInt() << ','
                << color[1].AsInt() << ','
                << color[2].AsInt() << ')';
        } else {
            res << "rgba("
                << color[0].AsInt() << ','
                << color[1].AsInt() << ','
                << color[2].AsInt() << ','
                << color[3].AsDouble() << ')';
        }
    }
    return res.str();
}


JsonReader::JsonReader(istream& in) : document_(move(Load(in))) {
    request_commands_ = &document_.GetRoot().AsDict().at("base_requests").AsArray();
    settings_commands_ = &document_.GetRoot().AsDict().at("render_settings").AsDict();
    stat_commands_ = &document_.GetRoot().AsDict().at("stat_requests").AsArray();
    routing_settings_commands_ = &document_.GetRoot().AsDict().at("routing_settings").AsDict();
}

void JsonReader::ApplyCommands(TransportCatalogue& transport_catalogue, MapRenderer& map_renderer, TransportRouter& router) const {
    Array stop_commands;
    Array bus_commands;

    DistributeRequests(transport_catalogue, stop_commands, bus_commands);
    ProcessStopCommands(stop_commands, transport_catalogue);
    ProcessBusCommands(bus_commands, transport_catalogue);
    ProcessSettingCommands(map_renderer);
    ProcessRouteSettingCommands(router);
}

void JsonReader::PrintJson(RequestHandler& request_handler, ostream& output) const {
    Array infos;
    
    for (const auto& stat_command : *stat_commands_) {
        Builder builder;       
        if (stat_command.AsDict().at("type"s) == "Stop"s) {
            
            if (!request_handler.GetBusesByStop(stat_command.AsDict().at("name"s).AsString()).has_value()) {
                builder.StartDict().Key("error_message"s).Value(string{"not found"s})
                                   .Key("request_id"s).Value(stat_command.AsDict().at("id"s))
                       .EndDict();
                infos.push_back(builder.Build().AsDict());
                continue;
            }
            
            vector<const Bus*> buses = *request_handler.GetBusesByStop(stat_command.AsDict().at("name"s).AsString());
            
            if (!buses.size()) {
                builder.StartDict().Key("buses"s).StartArray().EndArray()
                                   .Key("request_id"s).Value(stat_command.AsDict().at("id"s))
                       .EndDict();
            }
            else {
                sort(buses.begin(), buses.end(), [](const Bus* lhs, const Bus* rhs) {
                    return lhs->name < rhs->name;
                });
                Array buses_names;
                for (auto bus : buses) {
                    buses_names.push_back(bus->name);
                }
                builder.StartDict().Key("buses"s).StartArray();
                                    for (const auto bus : buses) {
                                        builder.Value(bus->name);
                                    }
                                    builder.EndArray()
                                   .Key("request_id"s).Value(stat_command.AsDict().at("id"s))
                       .EndDict();
            }
            if (!builder.Build().IsNull()) {
                infos.push_back(builder.Build().AsDict());
            }
        }
        else if (stat_command.AsDict().at("type"s) == "Bus"s) {
            auto bus_info = *request_handler.GetBusStat(stat_command.AsDict().at("name"s).AsString());
            if (!request_handler.GetBusStat(stat_command.AsDict().at("name"s).AsString()).has_value()) {
                
                builder.StartDict().Key("error_message"s).Value(string{"not found"s})
                                   .Key("request_id"s).Value(stat_command.AsDict().at("id"))
                       .EndDict();
            }
            else {
                builder.StartDict().Key("curvature"s).Value(bus_info.curvature)
                                   .Key("request_id"s).Value(stat_command.AsDict().at("id"s))
                                   .Key("route_length"s).Value(bus_info.route_length)
                                   .Key("stop_count"s).Value(static_cast<int>(bus_info.stops))
                                   .Key("unique_stop_count"s).Value(static_cast<int>(bus_info.unique_stops))
                       .EndDict();
            }
            if (!builder.Build().IsNull()) {
                infos.push_back(builder.Build().AsDict());
            }
        }
        else if (stat_command.AsDict().at("type"s) == "Map"s) {
            ostringstream map;
            request_handler.RenderMap().Render(map);
            builder.StartDict().Key("map"s).Value(map.str())
                               .Key("request_id"s).Value(stat_command.AsDict().at("id"))
                   .EndDict();

            if (!builder.Build().IsNull()) {
                infos.push_back(builder.Build().AsDict());
            }   
        }
        else {
            const auto& route_info = request_handler.GetRouteInfo(
                stat_command.AsDict().at("from").AsString(),
                stat_command.AsDict().at("to").AsString()
            );
                builder.StartDict()
                        .Key("request_id").Value(stat_command.AsDict().at("id").AsInt());
            if (route_info.has_value()) {
                builder.Key("total_time").Value(route_info->all_time)
                        .Key("items").StartArray();
                for (const auto& item : route_info->items) {
                    builder.StartDict();
                    builder.Key("time").Value(item.time)
                            .Key("type").Value(item.type);
                    if (item.type == "Wait") {
                        builder.Key("stop_name").Value(item.name);
                    } else if (item.type == "Bus") {
                        builder.Key("bus").Value(item.name)
                                .Key("span_count").Value(item.span_count);
                    }
                    builder.EndDict();
                }
                builder.EndArray();
            } else {
                builder.Key("error_message").Value(std::string("not found"));
            }
            
            builder.EndDict();

            if (!builder.Build().IsNull()) {
                infos.push_back(builder.Build().AsDict());
            }
        }
    }
    Document document{infos};
    Print(document, output);
}

void JsonReader::DistributeRequests(TransportCatalogue& transport_catalogue, Array& stop_commands, Array& bus_commands) const {
    for (const auto& request_command : *request_commands_) {
        auto request = request_command.AsDict();
        if (request.at("type"s) == "Stop"s) {
            transport_catalogue.AddStop({request.at("name"s).AsString(), {request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble()}});
            stop_commands.push_back(request_command);
        }
        else {
            bus_commands.push_back(request_command);
        }
    }
}

void JsonReader::ProcessStopCommands(const Array& stop_commands, TransportCatalogue& transport_catalogue) const {
    for (const auto& stop_command : stop_commands) {
        auto road_distances = stop_command.AsDict().at("road_distances"s).AsDict();
        if (road_distances.size()) {
            for (const auto& [stop, distance] : road_distances) {
                auto first_stop = transport_catalogue.FindStop(stop_command.AsDict().at("name"s).AsString());
                auto second_stop = transport_catalogue.FindStop(stop);
                transport_catalogue.SetStopsDistance(first_stop, second_stop, distance.AsInt());
            }
        }            
    }
}

void JsonReader::ProcessBusCommands(const Array& bus_commands, TransportCatalogue& transport_catalogue) const {
    for (const auto& bus_command : bus_commands) {
        vector<const Stop*> stops;

        auto stops_of_bus = bus_command.AsDict().at("stops"s).AsArray();
        for (const auto& stop : stops_of_bus) {
            auto stop_to_add = transport_catalogue.FindStop(stop.AsString());
            if (stop_to_add) {
                stops.push_back(stop_to_add);
            }
        }
        if (stops_of_bus.size() > 1 && !bus_command.AsDict().at("is_roundtrip"s).AsBool()) {
            for(auto it = stops_of_bus.rbegin() + 1; it != stops_of_bus.rend(); ++it) {
                auto stop_to_add = transport_catalogue.FindStop((*it).AsString());
                if (stop_to_add) {
                    stops.push_back(stop_to_add);
                }
            }
            transport_catalogue.AddBus({bus_command.AsDict().at("name"s).AsString(), stops, false});
            continue;
        }
        transport_catalogue.AddBus({bus_command.AsDict().at("name"s).AsString(), stops, true});
    }
}

void JsonReader::ProcessSettingCommands(MapRenderer& map_renderer) const {
    RenderSettings render_settings;
    render_settings.width = settings_commands_->at("width"s).AsDouble();
    render_settings.height = settings_commands_->at("height"s).AsDouble();
    render_settings.padding = settings_commands_->at("padding"s).AsDouble();
    render_settings.line_width = settings_commands_->at("line_width"s).AsDouble();
    render_settings.stop_radius = settings_commands_->at("stop_radius"s).AsDouble();
    render_settings.bus_label_font_size = settings_commands_->at("bus_label_font_size"s).AsInt();
    
    auto bus_label_offset_array = settings_commands_->at("bus_label_offset"s).AsArray();
    render_settings.bus_label_offset[0] = bus_label_offset_array[0].AsDouble();
    render_settings.bus_label_offset[1] = bus_label_offset_array[1].AsDouble();

    render_settings.stop_label_font_size = settings_commands_->at("stop_label_font_size"s).AsInt();
    
    auto stop_label_offset_array = settings_commands_->at("stop_label_offset"s).AsArray();
    render_settings.stop_label_offset[0] = stop_label_offset_array[0].AsDouble();
    render_settings.stop_label_offset[1] = stop_label_offset_array[1].AsDouble();

    render_settings.underlayer_color = GetColor(settings_commands_->at("underlayer_color"s));
    render_settings.underlayer_width = settings_commands_->at("underlayer_width"s).AsDouble();
    auto color_palette_array = settings_commands_->at("color_palette"s).AsArray();
    for (const auto& color : color_palette_array) {
        render_settings.color_palette.push_back(GetColor(color));
    }

    map_renderer.SetRenderSettings(render_settings);
}

void JsonReader::ProcessRouteSettingCommands(TransportRouter& router) const {
    int wait_time;
    double bus_avg_speed;
    for (const auto& [key, value] : *routing_settings_commands_) {
        if (key == "bus_velocity") {
            bus_avg_speed = value.AsDouble();
        } else if (key == "bus_wait_time") {
            wait_time = value.AsInt();
        }
    }
    router.SetSettings(wait_time, bus_avg_speed);
}
