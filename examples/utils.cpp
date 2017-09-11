
#include "utils.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

std::string read_file(const std::string& filename) {
    std::ifstream t{filename};
    if (!t) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

vtzero::layer get_layer(vtzero::vector_tile& tile, const std::string& layer_name_or_num) {
    if (layer_name_or_num.find_first_not_of("0123456789") == std::string::npos) {
        const auto num = std::atoi(layer_name_or_num.c_str());
        auto layer = tile[num];
        if (!layer) {
            std::cerr << "No such layer: " << num << '\n';
            std::exit(1);
        }
        return layer;
    } else {
        auto layer = tile[layer_name_or_num];
        if (!layer) {
            std::cerr << "No layer named '" << layer_name_or_num << "'.\n";
            std::exit(1);
        }
        return layer;
    }
}
