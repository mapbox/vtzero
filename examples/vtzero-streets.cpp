/*****************************************************************************

  Example program for vtzero library.

  vtzero-streets - Copy features from road_label layer if they have property
                   class="street". Output is always in file "streets.mvt".

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

static bool keep_feature(vtzero::feature& feature) {
    static const std::string key{"class"};
    static const std::string val{"street"};

    while (const auto& prop = feature.next_property()) {
        if (key == prop.key() && val == prop.value().string_value()) {
            return true;
        }
    }

    return false;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::string input_file{argv[1]};
    std::string output_file{"streets.mvt"};

    const auto data = read_file(input_file);

    vtzero::vector_tile tile{data};

    auto layer = get_layer(tile, "road_label");
    if (!layer) {
        std::cerr << "No 'road_label' layer found\n";
        return 1;
    }

    vtzero::tile_builder tb;
    vtzero::layer_builder layer_builder{tb, layer};

    while (auto feature = layer.next_feature()) {
        if (keep_feature(feature)) {
            layer_builder.add_feature(feature);
        }
    }

    std::string output = tb.serialize();

    write_data_to_file(output, output_file);
}

