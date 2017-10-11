
#include "utils.hpp"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
# include <io.h>
#else
# include <unistd.h>
#endif

std::string read_file(const std::string& filename) {
    std::ifstream t{filename};
    if (!t) {
        throw std::runtime_error{std::string{"Can not open file '"} + filename + "'"};
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

void write_data_to_file(const std::string& data, const std::string& filename) {
    int flags = O_WRONLY | O_CREAT | O_TRUNC;

#ifdef _WIN32
    flags |= O_BINARY;
#else
    flags |= O_CLOEXEC;
#endif

    const int fd = ::open(filename.c_str(), flags, 0644); // NOLINT clang-tidy: cppcoreguidelines-pro-type-vararg
    if (fd < 0) {
        throw std::runtime_error{"Can not open output file"};
    }

#ifdef _WIN32
    using write_size_type = unsigned int;
#else
    using write_size_type = std::size_t;
#endif

    const auto len = ::write(fd, data.c_str(), write_size_type(data.size()));
    if (static_cast<std::size_t>(len) != data.size()) {
        throw std::runtime_error{"Error writing to output file"};
    }
}

vtzero::layer get_layer(vtzero::vector_tile& tile, const std::string& layer_name_or_num) {
    vtzero::layer layer;
    char* str_end = nullptr;
    const long num = std::strtol(layer_name_or_num.c_str(), &str_end, 10); // NOLINT clang-tidy: google-runtime-int

    if (str_end == layer_name_or_num.data() + layer_name_or_num.size()) {
        if (num >= 0 && num < std::numeric_limits<long>::max()) { // NOLINT clang-tidy: google-runtime-int
            layer = tile[num];
            if (!layer) {
                std::cerr << "No such layer: " << num << '\n';
                std::exit(1);
            }
            return layer;
        }
    }

    layer = tile[layer_name_or_num];
    if (!layer) {
        std::cerr << "No layer named '" << layer_name_or_num << "'.\n";
        std::exit(1);
    }
    return layer;
}

