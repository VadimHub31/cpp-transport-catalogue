#include "request_handler.h"

using namespace std;
using namespace svg;

/*
* Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
* хотелось бы помещать ни в transport_catalogue, ни в json reader.
*
* Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
* можете оставить его пустым.
*/

const optional<BusInfo> RequestHandler::GetBusStat(const string_view& bus_name) const {
    auto found_bus = db_.FindBus(bus_name); 
    if (found_bus != nullptr) {
        return db_.GetBusInfo(*found_bus);
    }
    return nullopt;
}

const optional<vector<const Bus*>> RequestHandler::GetBusesByStop(const string_view& stop_name) const {
    auto found_stop = db_.FindStop(stop_name);
    if (found_stop != nullptr) {
        return db_.GetStopInfo(*found_stop);
    }
    return nullopt;
}

Document RequestHandler::RenderMap() const {
    return renderer_.Render(db_.GetBuses());
}