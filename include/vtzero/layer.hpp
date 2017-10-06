#ifndef VTZERO_LAYER_HPP
#define VTZERO_LAYER_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file layer.hpp
 *
 * @brief Contains the layer and layer_iterator classes.
 */

#include "exception.hpp"
#include "feature.hpp"
#include "geometry.hpp"
#include "property_value_view.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <iterator>
#include <vector>

namespace vtzero {

    /**
     * Iterator for iterating over features in a layer. You usually do not
     * create these but get them from calling vector_tile::begin()/end().
     */
    class layer_iterator {

        const layer* m_layer = nullptr;
        protozero::pbf_message<detail::pbf_layer> m_layer_reader;
        data_view m_data{};

        void next() {
            if (m_layer_reader.next(detail::pbf_layer::features,
                                    protozero::pbf_wire_type::length_delimited)) {
                m_data = m_layer_reader.get_view();
            } else {
                m_data = data_view{};
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
        layer_iterator() noexcept = default;

        /**
         * Construct feature iterator from specified vector tile data.
         *
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer_iterator(const layer* layer, const data_view& tile_data) :
            m_layer(layer),
            m_layer_reader(tile_data) {
            next();
        }

        /**
         * Dereference operator to get the feature.
         *
         * @returns feature
         */
        feature operator*() const {
            assert(m_data.data() != nullptr);
            return feature{m_layer, m_data};
        }

        /**
         * Prefix increment operator.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer_iterator& operator++() {
            next();
            return *this;
        }

        /**
         * Postfix increment operator.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer_iterator operator++(int) {
            const layer_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        /// Equality operator
        bool operator==(const layer_iterator& other) const noexcept {
            return m_data == other.m_data;
        }

        /// Inequality operator
        bool operator!=(const layer_iterator& other) const noexcept {
            return !(*this == other);
        }

    }; // layer_iterator

    /**
     * A layer according to spec 4.1. It contains a version, the extent,
     * and a name as well as a collection of features. Use the begin()/end()
     * methods to get an iterator for accessing the features.
     */
    class layer {

        data_view m_data{};
        uint32_t m_version = 1; // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
        uint32_t m_extent = 4096; // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70
        std::size_t m_num_features = 0;
        data_view m_name{};
        mutable std::vector<data_view> m_key_table;
        mutable std::vector<property_value_view> m_value_table;
        mutable std::size_t m_key_table_size = 0;
        mutable std::size_t m_value_table_size = 0;

        void initialize_tables() const {
            m_key_table.reserve(m_key_table_size);
            m_key_table_size = 0;

            m_value_table.reserve(m_value_table_size);
            m_value_table_size = 0;

            protozero::pbf_message<detail::pbf_layer> reader{m_data};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                        m_key_table.push_back(reader.get_view());
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                        m_value_table.emplace_back(reader.get_view());
                        break;
                    default:
                        reader.skip();
                        break;
                }
            }
        }

    public:

        using iterator = layer_iterator;
        using const_iterator = layer_iterator;

        /**
         * Construct an invalid layer object.
         */
        layer() = default;

        /**
         * Construct a layer object. This is usually not something done in
         * user code, because layers are created by the tile_iterator.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 and 2 are supported)
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        explicit layer(const data_view& data) :
            m_data(data) {
            protozero::pbf_message<detail::pbf_layer> reader{data};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_layer::version, protozero::pbf_wire_type::varint):
                        m_version = reader.get_uint32();
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::name, protozero::pbf_wire_type::length_delimited):
                        m_name = reader.get_view();
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited):
                        reader.skip(); // ignore features for now
                        ++m_num_features;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                        reader.skip();
                        ++m_key_table_size;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                        reader.skip();
                        ++m_value_table_size;
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
                }
            }

            // This library can only handle version 1 and 2.
            if (m_version < 1 || m_version > 2) {
                throw version_exception{m_version};
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.data() == nullptr) {
                throw format_exception{"missing name field in layer (spec 4.1)"};
            }
        }

        /**
         * Is this a valid layer? Valid layers are those not created from the
         * default constructor.
         */
        bool valid() const noexcept {
            return m_data.data() != nullptr;
        }

        /**
         * Is this a valid layer? Valid layers are those not created from the
         * default constructor.
         */
        explicit operator bool() const noexcept {
            return valid();
        }

        /**
         * Get a reference to the raw data this layer is created from.
         */
        data_view data() const noexcept {
            return m_data;
        }

        /**
         * Return the name of the layer.
         */
        data_view name() const noexcept {
            assert(valid());
            return m_name;
        }

        /**
         * Return the version of this layer.
         */
        std::uint32_t version() const noexcept {
            assert(valid());
            return m_version;
        }

        /**
         * Return the extent of this layer.
         */
        std::uint32_t extent() const noexcept {
            assert(valid());
            return m_extent;
        }

        /**
         * Return a reference to the key table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         */
        const std::vector<data_view>& key_table() const {
            if (m_key_table_size > 0) {
                initialize_tables();
            }
            return m_key_table;
        }

        /**
         * Return a reference to the value table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         */
        const std::vector<property_value_view>& value_table() const {
            if (m_value_table_size > 0) {
                initialize_tables();
            }
            return m_value_table;
        }

        /**
         * Get the property key with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws std::out_of_range if the index is out of range.
         */
        data_view key(index_value index) const {
            return key_table().at(index.value());
        }

        /**
         * Get the property value with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws std::out_of_range if the index is out of range.
         */
        property_value_view value(index_value index) const {
            return value_table().at(index.value());
        }

        /**
         * Does this layer contain any features?
         *
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_num_features == 0;
        }

        /**
         * The number of features in this layer.
         *
         * Complexity: Constant.
         */
        std::size_t size() const noexcept {
            return m_num_features;
        }

        /// Returns an iterator to the beginning of features.
        layer_iterator begin() const {
            return {this, m_data};
        }

        /// Returns an iterator to the end of features.
        layer_iterator end() const {
            return {};
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
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        feature operator[](uint64_t id) const {
            assert(valid());

            protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
            while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                const auto feature_data = layer_reader.get_view();
                protozero::pbf_message<detail::pbf_feature> feature_reader{feature_data};
                if (feature_reader.next(detail::pbf_feature::id, protozero::pbf_wire_type::varint)) {
                    if (feature_reader.get_uint64() == id) {
                        return feature{this, feature_data};
                    }
                }
            }

            return feature{};
        }

    }; // class layer

    inline property_view properties_iterator::operator*() const {
        return {m_layer->key(*m_it), m_layer->value(*std::next(m_it))};
    }

} // namespace vtzero

#endif // VTZERO_LAYER_HPP
