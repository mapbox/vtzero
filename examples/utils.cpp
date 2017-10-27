
#include "utils.hpp"

#include <fstream>
#include <iostream>

std::string read_file(const std::string& filename) {
    std::ifstream stream{filename, std::ios_base::in | std::ios_base::binary};
    if (!stream) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }

    stream.exceptions(std::ifstream::failbit);

    std::string buffer{std::istreambuf_iterator<char>(stream.rdbuf()),
                       std::istreambuf_iterator<char>()};
    stream.close();

    return buffer;
}

void write_data_to_file(const std::string& buffer, const std::string& filename) {
    std::ofstream stream{filename, std::ios_base::out | std::ios_base::binary};
    if (!stream) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }

    stream.exceptions(std::ifstream::failbit);

    stream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    stream.close();
}

vtzero::layer get_layer(const vtzero::vector_tile& tile, const std::string& layer_name_or_num) {
    vtzero::layer layer;
    char* str_end = nullptr;
    const long num = std::strtol(layer_name_or_num.c_str(), &str_end, 10); // NOLINT clang-tidy: google-runtime-int

    if (str_end == layer_name_or_num.data() + layer_name_or_num.size()) {
        if (num >= 0 && num < std::numeric_limits<long>::max()) { // NOLINT clang-tidy: google-runtime-int
            layer = tile.get_layer(static_cast<std::size_t>(num));
            if (!layer) {
                std::cerr << "No such layer: " << num << '\n';
                std::exit(1);
            }
            return layer;
        }
    }

    layer = tile.get_layer_by_name(layer_name_or_num);
    if (!layer) {
        std::cerr << "No layer named '" << layer_name_or_num << "'.\n";
        std::exit(1);
    }
    return layer;
}

