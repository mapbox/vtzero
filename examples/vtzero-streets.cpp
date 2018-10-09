/*****************************************************************************

  Example program for vtzero library.

  vtzero-streets - Copy features from road_label layer if they have an attribute
                   class="street". Output is always in file "streets.mvt".

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/output.hpp>
#include <vtzero/property_mapper.hpp>
#include <vtzero/vector_tile.hpp>

#include <iostream>
#include <string>

class filter_handler {

    bool m_found = false;

public:

    void attribute_key(const vtzero::data_view& key, std::size_t /*depth*/) {
        static const std::string mkey{"class"};
        m_found = (mkey == key);
    }

    template <typename T>
    void attribute_value(T /*value*/, std::size_t /*depth*/) {
        m_found = false;
    }

    bool attribute_value(const vtzero::data_view& value, std::size_t /*depth*/) {
        static const std::string mvalue{"street"};
        if (m_found && mvalue == value) {
            return false;
        }
        m_found = false;
        return true;
    }

    bool result() const noexcept {
        return m_found;
    }

}; // class filter_handler

static bool keep_feature(const vtzero::feature& feature) {
    filter_handler handler;
    return feature.decode_attributes(handler);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::string input_file{argv[1]};
    std::string output_file{"streets.mvt"};

    const auto data = read_file(input_file);

    try {
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
                vtzero::geometry_2d_feature_builder feature_builder{layer_builder};
                feature_builder.copy_id(feature);
                feature_builder.copy_geometry(feature);
                feature_builder.copy_attributes(feature);
                feature_builder.commit();
            }
        }

        std::string output = tb.serialize();
        write_data_to_file(output, output_file);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

