/*****************************************************************************

  Example program for vtzero library.

  vtzero-stats - Output some stats on layers

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/vector_tile.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

std::size_t geometries_size(const vtzero::layer& layer) {
    std::size_t sum = 0;

    layer.for_each_feature([&sum](vtzero::feature&& feature) {
        sum += feature.geometry().data().size();
        return true;
    });

    return sum;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::cout << "layer,num_features,raw_size,raw_geometries_size,key_table_size,value_table_size\n";
    try {
        const std::string input_file{argv[1]};
        const auto data = read_file(input_file);

        vtzero::vector_tile tile{data};

        while (const auto layer = tile.next_layer()) {
            std::cout.write(layer.name().data(), static_cast<std::streamsize>(layer.name().size()));
            std::cout << ','
                      << layer.num_features() << ','
                      << layer.data().size() << ','
                      << geometries_size(layer) << ','
                      << layer.key_table().size() << ','
                      << layer.value_table().size() << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
