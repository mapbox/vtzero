#ifndef VTZERO_LAYER_HPP
#define VTZERO_LAYER_HPP

#include "exception.hpp"
#include "feature.hpp"
#include "geometry.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <iterator>
#include <vector>

namespace vtzero {

    class feature_iterator {

        protozero::pbf_message<detail::pbf_layer> m_layer_reader;
        data_view m_data;

        void next() {
            try {
                if (m_layer_reader.next(detail::pbf_layer::features,
                                        protozero::pbf_wire_type::length_delimited)) {
                    m_data = m_layer_reader.get_view();
                } else {
                    m_data = data_view{};
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }
        }

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = feature;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        /**
         * Construct special "end" iterator.
         */
        feature_iterator() = default;

        /**
         * Construct feature iterator from specified vector tile data.
         *
         * @throws format_exception if the tile data is ill-formed.
         */
        feature_iterator(const data_view& tile_data) :
            m_layer_reader(tile_data),
            m_data() {
            next();
        }

        feature operator*() const {
            assert(m_data.data() != nullptr);
            return feature{m_data};
        }

        /**
         * @throws format_exception if the layer data is ill-formed.
         */
        feature_iterator& operator++() {
            next();
            return *this;
        }

        /**
         * @throws format_exception if the layer data is ill-formed.
         */
        feature_iterator operator++(int) {
            const feature_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const feature_iterator& other) const noexcept {
            return m_data == other.m_data;
        }

        bool operator!=(const feature_iterator& other) const noexcept {
            return !(*this == other);
        }

    }; // feature_iterator

    /**
     * A layer according to spec 4.1
     */
    class layer {

        data_view m_data;
        uint32_t m_version;
        uint32_t m_extent;
        data_view m_name;
        std::vector<data_view> m_key_table;
        std::vector<data_view> m_value_table;

    public:

        layer() :
            m_data(),
            m_version(0),
            m_extent(0),
            m_name() {
        }

        /**
         * Construct a layer object.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 and 2 are supported)
         */
        explicit layer(const data_view& data) :
            m_data(data),
            m_version(1), // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
            m_extent(4096), // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70
            m_name() {
            try {
                protozero::pbf_message<detail::pbf_layer> reader{data};
                while (reader.next()) {
                    switch (reader.tag_and_type()) {
                        case protozero::tag_and_type(detail::pbf_layer::version, protozero::pbf_wire_type::varint):
                            m_version = reader.get_uint32();
                            // This library can only handle version 1 and 2.
                            if (m_version < 1 || m_version > 2) {
                                throw version_exception{m_version};
                            }
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::name, protozero::pbf_wire_type::length_delimited):
                            m_name = reader.get_view();
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited):
                            reader.skip(); // ignore features for now
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                            m_key_table.push_back(reader.get_view());
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                            m_value_table.push_back(reader.get_view());
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::extent, protozero::pbf_wire_type::varint):
                            m_extent = reader.get_uint32();
                            break;
                        default:
                            throw format_exception{"unknown field in layer (tag=" +
                                                   std::to_string(static_cast<uint32_t>(reader.tag())) +
                                                   ", type=" +
                                                   std::to_string(static_cast<uint32_t>(reader.wire_type())) +
                                                   ")"};
                            break;
                    }
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.data() == nullptr) {
                throw format_exception{"missing name field in layer (spec 4.1)"};
            }
        }

        bool valid() const noexcept {
            return m_version != 0;
        }

        operator bool() const noexcept {
            return valid();
        }

        data_view data() const noexcept {
            return m_data;
        }

        data_view name() const noexcept {
            assert(valid());
            return m_name;
        }

        std::uint32_t version() const noexcept {
            assert(valid());
            return m_version;
        }

        std::uint32_t extent() const noexcept {
            assert(valid());
            return m_extent;
        }

        const std::vector<data_view>& key_table() const noexcept {
            return m_key_table;
        }

        const std::vector<data_view>& value_table() const noexcept {
            return m_value_table;
        }

        const data_view& key(uint32_t n) const {
            if (n >= m_key_table.size()) {
                throw format_exception{std::string{"key table index too large: "} + std::to_string(n)};
            }

            return m_key_table[n];
        }

        const data_view& value(uint32_t n) const {
            if (n >= m_value_table.size()) {
                throw format_exception{std::string{"value table index too large: "} + std::to_string(n)};
            }

            return m_value_table[n];
        }

        feature_iterator begin() const {
            return feature_iterator{m_data};
        }

        feature_iterator end() const {
            return feature_iterator{};
        }

        /**
         * Get the feature with the specified ID.
         *
         * Complexity: Linear in the number of features.
         *
         * @param id The ID to look for.
         * @returns Feature with the specified ID or the invalid feature if
         *          there is no feature with this ID.
         * @throws format_exception if the layer data is ill-formed.
         */
        feature get_feature(uint64_t id) const {
            assert(valid());

            try {
                protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
                while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                    const auto feature_data = layer_reader.get_view();
                    protozero::pbf_message<detail::pbf_feature> feature_reader{feature_data};
                    if (feature_reader.next(detail::pbf_feature::id, protozero::pbf_wire_type::varint)) {
                        if (feature_reader.get_uint64() == id) {
                            return feature{feature_data};
                        }
                    }
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            return feature{};
        }

        /**
         * Does this layer contain any features?
         *
         * Complexity: Constant.
         *
         * @throws format_exception if the layer data is ill-formed.
         */
        bool empty() const {
            try {
                protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
                if (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                    return false;
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }
            return true;
        }

        /**
         * The number of features in this layer.
         *
         * Complexity: Linear in the number of features.
         *
         * @throws format_exception if the layer data is ill-formed.
         */
        std::size_t size() const {
            assert(valid());
            std::size_t count = 0;

            try {
                protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
                while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                    layer_reader.skip();
                    ++count;
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            return count;
        }

    }; // class layer

    inline tag_view tags_iterator::operator*() const {
        assert(m_it != m_end);
        if (std::next(m_it) == m_end) {
            throw format_exception{"unpaired tag key/value indexes (spec 4.4)"};
        }
        return {m_layer->key(*m_it), m_layer->value(*std::next(m_it))};
    }

} // namespace vtzero

#endif // VTZERO_LAYER_HPP
