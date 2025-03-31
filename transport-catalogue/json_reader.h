#pragma once

#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"

/*
* Здесь можно разместить код наполнения транспортного справочника данными из JSON,
* а также код обработки запросов к базе и формирование массива ответов в формате JSON
*/

class JsonReader {
public:
    explicit JsonReader(std::istream& input);

    void ApplyCommands(transport_catalogue::TransportCatalogue& transport_catalogue, MapRenderer& map_renderer) const;
    
    void PrintJson(RequestHandler& request_handler, std::ostream& output) const;
    
private:
    json::Document document_;
    const json::Array* request_commands_;
    const json::Dict* settings_commands_;
    const json::Array* stat_commands_;

    void DistributeRequests(transport_catalogue::TransportCatalogue& transport_catalogue, json::Array& stop_commands, json::Array& bus_commands) const;
    void ProcessStopCommands(const json::Array& stop_commands, transport_catalogue::TransportCatalogue& transport_catalogue) const;
    void ProcessBusCommands(const json::Array& bus_commands, transport_catalogue::TransportCatalogue& transport_catalogue) const;
    void ProcessSettingCommands(MapRenderer& map_renderer) const;
};