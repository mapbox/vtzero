
#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/vector_tile.hpp>

#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

void print_help() {
    std::cout << "vtzero-filter [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n\n"
              << "Dump contents of vector tile.\n"
              << "\nOptions:\n"
              << "  -h, --help         This help message\n";
}

void print_usage(const char* command) {
    std::cerr << "Usage: " << command << " [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n";
    std::exit(1);
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
        print_usage(argv[0]);
    }

    const auto data = read_file(argv[optind]);

    vtzero::vector_tile tile{data};

    auto layer = get_layer(tile, argv[optind + 1]);
    std::cerr << "Found layer: " << std::string(layer.name()) << "\n";

    std::string output;
    if (remaining_args == 2) {
        vtzero::tile_builder tb;
        tb.add_layer(layer.data());
        tb.serialize(output);
    } else {
        char* str_end = nullptr;
        const long id = std::strtol(argv[optind + 2], &str_end, 10); // NOLINT clang-tidy: google-runtime-int
        if (str_end != argv[optind + 2] + std::strlen(argv[optind + 2])) {
            print_usage(argv[0]);
        }
        if (id < 0 || id > std::numeric_limits<long>::max()) { // NOLINT clang-tidy: google-runtime-int
            print_usage(argv[0]);
        }

        auto feature = layer.get_feature_by_id(static_cast<uint32_t>(id));
        if (!feature.valid()) {
            std::cerr << "No feature with that id\n";
            std::exit(1);
        }
        vtzero::tile_builder tb;
        vtzero::layer_builder layer_builder{tb, layer};
        layer_builder.add_feature(feature);
        tb.serialize(output);
    }

    write_data_to_file(output, "filtered.mvt");
}

