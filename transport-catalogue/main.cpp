#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>
#include <fstream>

using namespace transport_catalogue;
using namespace std;

int main() {
    TransportCatalogue transport_catalogue;
    MapRenderer map_renderer;
    RequestHandler request_handler(transport_catalogue, map_renderer);  

    // std::ifstream file("input.txt");

    // // Проверяем, удалось ли открыть файл
    // if (!file.is_open()) {
    //     std::cerr << "Не удалось открыть файл!" << std::endl;
    //     return 1;
    // }

    // std::ofstream out("output.txt");

    // if (!out.is_open()) {
    //     std::cerr << "Не удалось открыть файл!" << std::endl;
    //     return 1;
    // }

    JsonReader reader(cin);
    reader.ApplyCommands(transport_catalogue, map_renderer);
    //request_handler.RenderMap().Render(out);
    reader.PrintJson(request_handler, cout);
}