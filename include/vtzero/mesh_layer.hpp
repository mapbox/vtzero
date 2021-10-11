#pragma once

#include "protozero/pbf_builder.hpp"
#include "protozero/pbf_message.hpp"
#include "vtzero/builder.hpp"
#include <cassert>
#include <array>
#include <unistd.h>

namespace vtzero {

struct Vertex {
    std::array<uint16_t, 4> position;
};

struct feature_mesh_data {
    std::vector<char>    draco_compressed_data;
    unsigned int            elevation_min;
    unsigned int            elevation_max;
    unsigned int            precision;
};

class mesh_ext {
public:
    static constexpr uint32_t id = 18;

    enum class tag : protozero::pbf_tag_type {
        draco_compressed_data = 1,
        elevation_min = 2,
        elevation_max = 3,
        precision = 4
    };

    static std::string encode(const feature_mesh_data &mesh) {
        std::string data;
        protozero::pbf_builder<tag> builder { data };


        builder.add_message(tag::draco_compressed_data, mesh.draco_compressed_data.data(), mesh.draco_compressed_data.size());
        builder.add_int32(tag::elevation_min, mesh.elevation_min);
        builder.add_int32(tag::elevation_max, mesh.elevation_max);
        builder.add_int32(tag::precision, mesh.precision);
        return data;
    }

    static bool decode(protozero::data_view view, feature_mesh_data &mesh) {
        protozero::pbf_message<tag> message { view };

        while (message.next()) {
            switch (message.tag_and_type()) {
                case protozero::tag_and_type(tag::draco_compressed_data, protozero::pbf_wire_type::length_delimited): {
                    auto data = message.get_bytes();
                    mesh.draco_compressed_data.reserve(data.size());
                    for (auto it = data.begin(); it != data.end(); ++it) {
                        uint32_t packed_data = *it;
                        mesh.draco_compressed_data.push_back(packed_data);
                    }
                break;
                }
                case protozero::tag_and_type(tag::elevation_max, protozero::pbf_wire_type::varint):
                    mesh.elevation_max = message.get_int32();
                    break;
                case protozero::tag_and_type(tag::elevation_min, protozero::pbf_wire_type::varint):
                    mesh.elevation_min = message.get_int32();
                    break;
                case protozero::tag_and_type(tag::precision, protozero::pbf_wire_type::varint):
                    mesh.precision = message.get_int32();
                    break;
                default:
                    printf("mesh_ext: skipping decode!\n");
                    message.skip();
            }
        }
        return true;
    }
};

class mesh_layer_builder : public vtzero::layer_builder {
public:
    mesh_layer_builder(vtzero::tile_builder &tile,
                       const std::string &name,
                       uint32_t version = 3,
                       uint32_t extent = 4096)
        : layer_builder(tile, name, version, extent) {
    }

    bool set_mesh(const feature_mesh_data &mesh) {
        add_extension(mesh_ext::id, mesh_ext::encode(mesh));
        return true;
    }
};

} // namespace vtzero
