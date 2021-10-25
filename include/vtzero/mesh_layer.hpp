#pragma once

#include "protozero/pbf_builder.hpp"
#include "protozero/pbf_message.hpp"
#include "vtzero/builder.hpp"
#include <cassert>
#include <array>
#include <unistd.h>
#include <cstddef>

namespace vtzero {

struct feature_mesh_data {
    std::vector<unsigned char>          vertex;
    std::vector<unsigned char>          index;
    std::vector<uint32_t>               indices_ranges;
    unsigned int                        vertex_count;
    unsigned int                        index_count;
    unsigned int                        precision;
};

class mesh_ext {
public:
    static constexpr uint32_t id = 18;

    enum class tag : protozero::pbf_tag_type {
        vertex = 1,
        index = 2,
        precision = 3,
        vertex_count = 4,
        index_count = 5,
        indices_ranges = 6
    };

    static std::string encode(const feature_mesh_data &mesh) {
        std::string data;
        protozero::pbf_builder<tag> builder { data };

        builder.add_message(tag::vertex, (char*)mesh.vertex.data(), mesh.vertex.size());
        builder.add_message(tag::index, (char*)mesh.index.data(), mesh.index.size());
        builder.add_packed_uint32(tag::indices_ranges, std::begin(mesh.indices_ranges), std::end(mesh.indices_ranges));
        builder.add_uint32(tag::vertex_count, mesh.vertex_count);
        builder.add_uint32(tag::index_count, mesh.index_count);
        builder.add_uint32(tag::precision, mesh.precision);
        return data;
    }

    static bool decode(protozero::data_view view, feature_mesh_data &mesh) {
        protozero::pbf_message<tag> message { view };

        while (message.next()) {
            switch (message.tag_and_type()) {
                case protozero::tag_and_type(tag::vertex, protozero::pbf_wire_type::length_delimited): {
                    auto data = message.get_bytes();
                    mesh.vertex.reserve(data.size());
                    for (auto it = data.begin(); it != data.end(); ++it) {
                        char packed_data = *it;
                        mesh.vertex.push_back((unsigned char)packed_data);
                    }
                break;
                }
                case protozero::tag_and_type(tag::index, protozero::pbf_wire_type::length_delimited): {
                    auto data = message.get_bytes();
                    mesh.index.reserve(data.size());
                    for (auto it = data.begin(); it != data.end(); ++it) {
                        char packed_data = *it;
                        mesh.index.push_back((unsigned char)packed_data);
                    }
                break;
                }
                case protozero::tag_and_type(tag::vertex_count, protozero::pbf_wire_type::varint):
                    mesh.vertex_count = message.get_uint32();
                    break;
                case protozero::tag_and_type(tag::index_count, protozero::pbf_wire_type::varint):
                    mesh.index_count = message.get_uint32();
                    break;
                case protozero::tag_and_type(tag::precision, protozero::pbf_wire_type::varint):
                    mesh.precision = message.get_uint32();
                    break;
                case protozero::tag_and_type(tag::indices_ranges, protozero::pbf_wire_type::length_delimited): {
                    auto pi = message.get_packed_uint32();
                    for (auto it = pi.begin(); it != pi.end(); ++it) {
                        mesh.indices_ranges.push_back(*it);
                    }
                    break;
                }
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
