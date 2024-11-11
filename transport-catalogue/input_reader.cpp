#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <string>

namespace transport_catalogue {

namespace detail {
/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

std::string_view DistancesOutOfDescription(std::string_view command_description) {
    auto first_comma = command_description.find_first_of(',');
    auto second_comma = command_description.find_first_of(',', first_comma + 1);
    if (second_comma != std::string::npos) {
        auto start_dist = command_description.find_first_not_of(", ", second_comma);
        auto end_dist = command_description.find_last_not_of(", ");
        auto distances = command_description.substr(start_dist, end_dist - start_dist + 1);
        return distances;
    }
    return {};
}

std::vector<std::string_view> DistVec(std::string_view distances) {
    std::vector<std::string_view> result;
    auto start = distances.find_first_not_of(", ");
    auto first_comma = distances.find_first_of(',');
    while (first_comma != std::string::npos) {
        result.push_back(distances.substr(start, first_comma - start));
        start = distances.find_first_not_of(", ", first_comma);
        first_comma = distances.find_first_of(',', first_comma + 1);
    }
    result.push_back(distances.substr(start, distances.find_last_not_of(' ') - start + 1));
    return result;
}

std::unordered_map<std::string_view, int> ParseDistances(std::string_view command_description) {
    std::unordered_map<std::string_view, int> result;
    auto distance_out_of_description = DistancesOutOfDescription(command_description);
    if (distance_out_of_description.size() == 0) {
        return {};
    }
    auto vector_of_distances = DistVec(distance_out_of_description);
    for (auto distance : vector_of_distances) {
        auto m_pos = distance.find_first_of('m');
        int distancee = std::stoi(std::string{distance.substr(0, m_pos)});
        auto first_space = distance.find_first_of(' ');
        auto second_space = distance.find_first_of(' ', first_space + 1);
        auto stop = distance.substr(second_space + 1, distance.find_last_not_of(' ') - second_space);
        result[stop] = distancee;
    }

    return result;
}

} //namespace detail

using namespace detail;

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    // Реализуйте метод самостоятельно
    std::vector<CommandDescription> bus_commands;
    std::vector<CommandDescription> stop_commands;

    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            catalogue.AddStop({command.id, ParseCoordinates(command.description)});
            stop_commands.push_back(command);
        }
        else if (command.command == "Bus") {
            bus_commands.push_back(command);
        }
    }

    for (const auto& stop_command : stop_commands) {
        auto distances = ParseDistances(stop_command.description);
        if (distances.size() != 0) {
            for (const auto& [stop, distance] : distances) {
                auto first_stop = catalogue.FindStop(stop_command.id);
                auto second_stop = catalogue.FindStop(stop);
                catalogue.SetStopsDistance({first_stop, second_stop}, distance);
            }
        }
    }

    for (const auto& bus_command : bus_commands) { 
        std::vector<const Stop*> stops;  

        for (const auto& stop : ParseRoute(bus_command.description)) { 
            auto stop_to_add = catalogue.FindStop(stop); 
            stops.push_back(stop_to_add); 
        } 
        catalogue.AddBus({bus_command.id, stops}); 
    } 
}

}
