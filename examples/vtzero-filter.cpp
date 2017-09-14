
#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/reader.hpp>

#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>

void print_help() {
    std::cout << "vtzero-filter [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n\n"
              << "Dump contents of vector tile.\n"
              << "\nOptions:\n"
              << "  -h, --help         This help message\n";
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",   no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    while (true) {
        const int c = getopt_long(argc, argv, "hlt", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(0);
            default:
                std::exit(1);
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args < 2 || remaining_args > 3) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n";
        std::exit(1);
    }

    const auto data = read_file(argv[optind]);

    vtzero::vector_tile tile{data};

    auto layer = get_layer(tile, argv[optind + 1]);

    if (remaining_args == 2) {
        vtzero::tile_builder tb;
        tb.add_layer(layer.data());
        std::cout << tb.serialize();
    } else {
        const uint32_t id = std::atoi(argv[optind + 2]);
        auto feature = layer.get_feature(id);
        if (!feature.valid()) {
            std::cerr << "No feature with that id\n";
            std::exit(1);
        }
        vtzero::tile_builder tb;
        vtzero::layer_builder layer_builder{tb, layer};
        layer_builder.add_feature(feature, layer);
        std::cout << tb.serialize();
    }
}

