#pragma once

#include "protozero/pbf_builder.hpp"
#include "protozero/pbf_message.hpp"
#include "vtzero/builder.hpp"
#include <cassert>

namespace vtzero {

struct elevation_data {
    bool elevation_per_feature = true;
     // store elevation min/max pairs for feature. max is delta to min
    bool elevation_min_max = false;
    int32_t elevation_precision = 1;
    int32_t elevation_min = 0, elevation_max = 0;
    // NB layer_feature_count == elevations.size()
    std::vector<int32_t> elevations;
    // some special data to
    uint32_t layer_coordinates_count;
};

class elevation_ext {
public:
    static constexpr uint32_t id = 17;

    enum class tag : protozero::pbf_tag_type {
        elevation_per_feature = 1,
        elevation_min_max = 2,
        elevation_precision = 4,
        elevation_min = 8,
        elevation_max = 9,
        elevations_count = 10,
        elevations = 11,
        layer_coordinates_count = 20,
    };

    static std::string encode(const elevation_data &elevation) {
        std::string data;
        protozero::pbf_builder<tag> builder { data };

        builder.add_bool(tag::elevation_per_feature, elevation.elevation_per_feature);
        builder.add_bool(tag::elevation_min_max, elevation.elevation_min_max);

        builder.add_int32(tag::elevation_precision, elevation.elevation_precision);
        builder.add_int32(tag::elevation_min, elevation.elevation_min);
        builder.add_int32(tag::elevation_max, elevation.elevation_max);

        if (!elevation.elevations.empty()) {
            builder.add_uint32(tag::elevations_count, elevation.elevations.size());
            builder.add_packed_sint32(tag::elevations, std::begin(elevation.elevations), std::end(elevation.elevations));
        }

        builder.add_uint32(tag::layer_coordinates_count, elevation.layer_coordinates_count);

        return data;
    }

    static bool decode(protozero::data_view view, elevation_data &elevation) {
        protozero::pbf_message<tag> message { view };

        while (message.next()) {
            switch (message.tag_and_type()) {
                case protozero::tag_and_type(tag::elevation_per_feature, protozero::pbf_wire_type::varint):
                    elevation.elevation_per_feature = message.get_bool();
                    break;
                case protozero::tag_and_type(tag::elevation_min_max, protozero::pbf_wire_type::varint):
                    elevation.elevation_min_max = message.get_bool();
                    break;
                case protozero::tag_and_type(tag::elevation_precision, protozero::pbf_wire_type::varint):
                    elevation.elevation_precision = message.get_int32();
                    break;
                case protozero::tag_and_type(tag::elevation_min, protozero::pbf_wire_type::varint):
                    elevation.elevation_min = message.get_int32();
                    break;
                case protozero::tag_and_type(tag::elevation_max, protozero::pbf_wire_type::varint):
                    elevation.elevation_max = message.get_int32();
                    break;
                case protozero::tag_and_type(tag::elevations_count, protozero::pbf_wire_type::varint):
                    elevation.elevations.reserve(message.get_uint32());
                    break;
                case protozero::tag_and_type(tag::elevations, protozero::pbf_wire_type::length_delimited): {
                    auto pi = message.get_packed_sint32();
                    for (auto it = pi.begin(); it != pi.end(); ++it) {
                        elevation.elevations.push_back(*it);
                    }
                    break;
                }
                case protozero::tag_and_type(tag::layer_coordinates_count, protozero::pbf_wire_type::varint):
                    elevation.layer_coordinates_count = message.get_uint32();
                    break;
                default:
                    printf("elevation_ext: skipping decode!\n");
                    message.skip();
            }
        }
        return true;
    }
};

class elevation_layer_builder : public vtzero::layer_builder {
public:
    elevation_layer_builder(vtzero::tile_builder &tile,
                       const std::string &name,
                       uint32_t version = 2,
                       uint32_t extent = 4096)
        : layer_builder(tile, name, version, extent) {
    }

    bool set_elevation(const elevation_data &elevation) {
        add_extension(elevation_ext::id, elevation_ext::encode(elevation));
        return true;
    }
};

} // namespace vtzero
