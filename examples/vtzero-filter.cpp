/*****************************************************************************

  Example program for vtzero library.

  vtzero-filter - Copy parts of a vector tile into a new tile.

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/vector_tile.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <string>

static void print_help() {
    std::cout << "vtzero-filter [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n\n"
              << "Filter contents of vector tile.\n"
              << "Options:\n"
              << "  -h, --help         This help message\n"
              << "  -o, --output=FILE  Write output to file FILE\n";
}

static void print_usage(const char* command) {
    std::cerr << "Usage: " << command << " [OPTIONS] VECTOR-TILE LAYER-NUM|LAYER-NAME [ID]\n";
}

int main(int argc, char* argv[]) {
    std::string output_file{"filtered.mvt"};

    static struct option long_options[] = {
        {"help",         no_argument, nullptr, 'h'},
        {"output", required_argument, nullptr, 'o'},
        {nullptr, 0, nullptr, 0}
    };

    while (true) {
        const int c = getopt_long(argc, argv, "ho:", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                return 0;
            case 'o':
                output_file = optarg;
                break;
            default:
                return 1;
        }
    }

    const int remaining_args = argc - optind;
    if (remaining_args < 2 || remaining_args > 3) {
        print_usage(argv[0]);
        return 1;
    }

    const auto data = read_file(argv[optind]);

    vtzero::vector_tile tile{data};

    auto layer = get_layer(tile, argv[optind + 1]);
    std::cerr << "Found layer: " << std::string(layer.name()) << "\n";

    vtzero::tile_builder tb;

    if (remaining_args == 2) {
        tb.add_existing_layer(layer);
    } else {
        std::string idstr{argv[optind + 2]};
        char* str_end = nullptr;
        const int64_t id = std::strtoll(idstr.c_str(), &str_end, 10);
        if (str_end != idstr.c_str() + idstr.size()) {
            std::cerr << "Feature ID must be numeric.\n";
            return 1;
        }
        if (id < 0) {
            std::cerr << "Feature ID must be >= 0.\n";
            return 1;
        }

        const auto feature = layer.get_feature_by_id(static_cast<uint64_t>(id));
        if (!feature.valid()) {
            std::cerr << "No feature with that id: " << id << '\n';
            return 1;
        }

        vtzero::layer_builder layer_builder{tb, layer};
        layer_builder.add_feature(feature);
    }

    std::string output = tb.serialize();

    write_data_to_file(output, output_file);
}

