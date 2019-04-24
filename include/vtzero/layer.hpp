#ifndef VTZERO_LAYER_HPP
#define VTZERO_LAYER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file layer.hpp
 *
 * @brief Contains the layer class.
 */

#include "detail/attributes.hpp"
#include "detail/geometry.hpp"
#include "detail/util.hpp"
#include "exception.hpp"
#include "feature.hpp"
#include "layer_table.hpp"
#include "property_value.hpp"
#include "scaling.hpp"
#include "tile.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace vtzero {

    /**
     * @brief For iterating over all features in a vector tile layer.
     *
     * Usage:
     * @code
     * vtzero::layer layer = ...;
     * for (auto feature : layer) {
     *   ...
     * }
     * @endcode
     */
    class feature_iterator {

        data_view m_data{};
        const layer* m_layer = nullptr;
        std::size_t m_feature_num = 0;

        void skip_non_features() {
            protozero::pbf_message<detail::pbf_layer> reader{m_data};
            while (reader.next()) {
                if (reader.tag_and_type() ==
                        protozero::tag_and_type(detail::pbf_layer::features,
                                                protozero::pbf_wire_type::length_delimited)) {
                    return;
                }
                reader.skip();
                m_data = reader.data();
            }
            m_data = reader.data();
        }

    public:

        /// @cond usual iterator functions not documented

        using iterator_category = std::forward_iterator_tag;
        using value_type        = feature;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        feature_iterator() noexcept = default;

        feature_iterator(data_view data, const layer* layer) :
            m_data(data),
            m_layer(layer) {
            skip_non_features();
        }

        feature operator*() const;

        feature_iterator& operator++() {
            if (!m_data.empty()) {
                protozero::pbf_message<detail::pbf_layer> reader{m_data};
                if (reader.next(detail::pbf_layer::features,
                                protozero::pbf_wire_type::length_delimited)) {
                    reader.skip();
                    m_data = reader.data();
                    ++m_feature_num;
                    skip_non_features();
                }
            }
            return *this;
        }

        feature_iterator operator++(int) {
            const feature_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        friend bool operator==(const feature_iterator& lhs, const feature_iterator& rhs) noexcept {
            return lhs.m_data == rhs.m_data;
        }

        friend bool operator!=(const feature_iterator& lhs, const feature_iterator& rhs) noexcept {
            return !(lhs == rhs);
        }

        /// @endcond

    }; // class feature_iterator

    /**
     * @brief A vector tile layer
     *
     * A layer according to spec 4.1. It contains a version, the extent,
     * and a name. For the most efficient way to access the features in this
     * layer use a range-for loop:
     *
     * @code
     *   std::string data = ...;
     *   const vtzero::vector_tile tile{data};
     *   const layer = *tile.begin();
     *   for (const auto feature : layer) {
     *     ...
     *   }
     * @endcode
     *
     * If you know the ID of a feature, you can get it directly with
     * @code
     *   layer.get_feature_by_id(7);
     * @endcode
     *
     * Note that the layer class uses mutable members inside to cache the
     * key and value tables. It can not be used safely in several threads
     * at once!
     */
    class layer {

        data_view m_data{};
        tile m_tile{};
        std::size_t m_layer_num = 0;
        std::size_t m_num_features = 0;
        data_view m_name{};
        protozero::pbf_message<detail::pbf_layer> m_layer_reader{m_data};

        // Elevation scaling
        scaling m_elevation_scaling{};

        // Geometric attribute scalings
        std::vector<scaling> m_attribute_scalings{};

        mutable std::vector<data_view> m_key_table;
        mutable std::vector<property_value> m_value_table;
        mutable std::vector<data_view> m_string_table;

        mutable std::size_t m_key_table_size = 0;
        mutable std::size_t m_value_table_size = 0;
        mutable std::size_t m_string_table_size = 0;

        layer_table<double> m_double_table;
        layer_table<float> m_float_table;
        layer_table<uint64_t> m_int_table;

        uint32_t m_version = 1; // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
        uint32_t m_extent = 4096; // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70

        void initialize_tables() const {
            m_key_table.reserve(m_key_table_size);
            m_key_table_size = 0;

            m_value_table.reserve(m_value_table_size);
            m_value_table_size = 0;

            m_string_table.reserve(m_string_table_size);
            m_string_table_size = 0;

            protozero::pbf_message<detail::pbf_layer> reader{m_data};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                        m_key_table.push_back(reader.get_view());
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                        m_value_table.emplace_back(reader.get_view());
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::string_values, protozero::pbf_wire_type::length_delimited):
                        m_string_table.emplace_back(reader.get_view());
                        break;
                    default:
                        reader.skip(); // ignore unknown fields
                }
            }
        }

    public:

        /**
         * Construct an invalid layer object.
         */
        layer() = default;

        /**
         * Construct a layer object. This is usually not something done in
         * user code, because layers are created by the layer_iterator or
         * the get_layer_* functions of the vector_tile class.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 to 3 are supported)
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        explicit layer(const data_view data, std::size_t layer_num) :
            m_data(data),
            m_layer_num(layer_num) {
            protozero::pbf_message<detail::pbf_layer> reader{data};
            uint32_t x = 0;
            uint32_t y = 0;
            uint32_t zoom = 0;
            bool has_x_y_zoom = false;
            while (reader.next()) {
                switch (reader.tag()) {
                    case detail::pbf_layer::version:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Layer version has wrong protobuf type", m_layer_num};
                        }
                        m_version = reader.get_uint32();
                        break;
                    case detail::pbf_layer::name:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer name has wrong protobuf type", m_layer_num};
                        }
                        m_name = reader.get_view();
                        break;
                    case detail::pbf_layer::features:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer features have wrong protobuf type", m_layer_num};
                        }
                        reader.skip(); // ignore features for now
                        ++m_num_features;
                        break;
                    case detail::pbf_layer::keys:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer keys have wrong protobuf type", m_layer_num};
                        }
                        reader.skip();
                        ++m_key_table_size;
                        break;
                    case detail::pbf_layer::values:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer values have wrong protobuf type", m_layer_num};
                        }
                        reader.skip();
                        ++m_value_table_size;
                        break;
                    case detail::pbf_layer::extent:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Layer extent has wrong protobuf type", m_layer_num};
                        }
                        m_extent = reader.get_uint32();
                        break;
                    case detail::pbf_layer::string_values:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer string_values have wrong protobuf type", m_layer_num};
                        }
                        reader.skip();
                        ++m_string_table_size;
                        break;
                    case detail::pbf_layer::double_values:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer double_values have wrong protobuf type", m_layer_num};
                        }
                        if (!m_double_table.empty()) {
                            throw format_exception{"More than one double table in layer", m_layer_num};
                        }
                        m_double_table = layer_table<double>{reader.get_view(), m_layer_num};
                        break;
                    case detail::pbf_layer::float_values:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer float_values have wrong protobuf type", m_layer_num};
                        }
                        if (!m_float_table.empty()) {
                            throw format_exception{"More than one float table in layer", m_layer_num};
                        }
                        m_float_table = layer_table<float>{reader.get_view(), m_layer_num};
                        break;
                    case detail::pbf_layer::int_values:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer int_values have wrong protobuf type", m_layer_num};
                        }
                        if (!m_int_table.empty()) {
                            throw format_exception{"More than one int table in layer", m_layer_num};
                        }
                        m_int_table = layer_table<uint64_t>{reader.get_view(), m_layer_num};
                        break;
                    case detail::pbf_layer::elevation_scaling:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer elevation_scaling has wrong protobuf type", m_layer_num};
                        }
                        m_elevation_scaling = scaling{reader.get_view()};
                        break;
                    case detail::pbf_layer::attribute_scalings:
                        if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                            throw format_exception{"Layer attribute_scalings have wrong protobuf type", m_layer_num};
                        }
                        m_attribute_scalings.emplace_back(reader.get_view());
                        break;
                    case detail::pbf_layer::tile_x:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Layer tile_x has wrong protobuf type", m_layer_num};
                        }
                        x = reader.get_uint32();
                        has_x_y_zoom = true;
                        break;
                    case detail::pbf_layer::tile_y:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Layer tile_y has wrong protobuf type", m_layer_num};
                        }
                        y = reader.get_uint32();
                        has_x_y_zoom = true;
                        break;
                    case detail::pbf_layer::tile_zoom:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Layer tile_zoom has wrong protobuf type", m_layer_num};
                        }
                        zoom = reader.get_uint32();
                        if (zoom >= tile::max_zoom) {
                            throw format_exception{"Zoom level in layer > " + std::to_string(tile::max_zoom) + " (spec 4.1)", m_layer_num};
                        }
                        has_x_y_zoom = true;
                        break;
                    default:
                        reader.skip();
                }
            }

            // This library can only handle version 1 to 3.
            if (m_version < 1 || m_version > 3) {
                throw version_exception{m_version, m_layer_num};
            }

            // Check for vt3 components in vt2 layers.
            if (m_version <= 2) {
                if (m_string_table_size > 0) {
                    throw format_exception{"String table in layer with version <= 2", m_layer_num};
                }
                if (!m_double_table.empty()) {
                    throw format_exception{"Double table in layer with version <= 2", m_layer_num};
                }
                if (!m_float_table.empty()) {
                    throw format_exception{"Float table in layer with version <= 2", m_layer_num};
                }
                if (!m_int_table.empty()) {
                    throw format_exception{"Int table in layer with version <= 2", m_layer_num};
                }
                if (m_elevation_scaling != scaling{}) {
                    throw format_exception{"Elevation scaling message in layer with version <= 2", m_layer_num};
                }
                if (!m_attribute_scalings.empty()) {
                    throw format_exception{"Attribute scaling message in layer with version <= 2", m_layer_num};
                }
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.empty()) {
                throw format_exception{"Missing name in layer (spec 4.1)", m_layer_num};
            }

            if (has_x_y_zoom) {
                if (x >= detail::num_tiles_in_zoom(zoom)) {
                    throw format_exception{"Tile x value in layer out of range (0 - " + std::to_string(detail::num_tiles_in_zoom(zoom) - 1) + ") (spec 4.1)", m_layer_num};
                }

                if (y >= detail::num_tiles_in_zoom(zoom)) {
                    throw format_exception{"Tile y value in layer out of range (0 - " + std::to_string(detail::num_tiles_in_zoom(zoom) - 1) + ") (spec 4.1)", m_layer_num};
                }

                m_tile = {x, y, zoom, m_extent};
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
         * Return the (zero-based) index of this layer in the vector tile.
         */
        std::size_t layer_num() const noexcept {
            return m_layer_num;
        }

        /**
         * Return the name of the layer.
         *
         * @pre @code valid() @endcode
         */
        data_view name() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_name;
        }

        /**
         * Return the version of this layer.
         *
         * @pre @code valid() @endcode
         */
        std::uint32_t version() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_version;
        }

        /**
         * Return the tile of this layer.
         *
         * @pre @code valid() @endcode
         */
        tile get_tile() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_tile;
        }

        /**
         * Return the extent of this layer.
         *
         * @pre @code valid() @endcode
         */
        std::uint32_t extent() const noexcept {
            vtzero_assert_in_noexcept_function(valid());

            return m_extent;
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
        std::size_t num_features() const noexcept {
            return m_num_features;
        }

        /**
         * Return a reference to the key table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @pre @code valid() @endcode
         */
        const std::vector<data_view>& key_table() const {
            vtzero_assert(valid());

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
         *
         * @pre @code valid() @endcode
         */
        const std::vector<property_value>& value_table() const {
            vtzero_assert(valid());

            if (m_value_table_size > 0) {
                initialize_tables();
            }
            return m_value_table;
        }

        /**
         * Return a reference to the string value table.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @pre @code valid() @endcode
         * @pre @code version() == 3 @endcode
         */
        const std::vector<data_view>& string_table() const {
            vtzero_assert(valid());
            vtzero_assert(version() == 3);

            if (m_string_table_size > 0) {
                initialize_tables();
            }
            return m_string_table;
        }

        /**
         * Return a reference to the double value table.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         * @pre @code version() == 3 @endcode
         */
        const layer_table<double>& double_table() const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            vtzero_assert_in_noexcept_function(version() == 3);
            return m_double_table;
        }

        /**
         * Return a reference to the float value table.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         * @pre @code version() == 3 @endcode
         */
        const layer_table<float>& float_table() const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            vtzero_assert_in_noexcept_function(version() == 3);
            return m_float_table;
        }

        /**
         * Return a reference to the int value table.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         * @pre @code version() == 3 @endcode
         */
        const layer_table<uint64_t>& int_table() const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            vtzero_assert_in_noexcept_function(version() == 3);
            return m_int_table;
        }

        /**
         * Return the size of the key table. This returns the correct value
         * whether the key table was already built or not.
         *
         * Complexity: Constant.
         *
         * @returns Size of the key table.
         */
        std::size_t key_table_size() const noexcept {
            return m_key_table_size > 0 ? m_key_table_size : m_key_table.size();
        }

        /**
         * Return the size of the value table. This returns the correct value
         * whether the value table was already built or not.
         *
         * Complexity: Constant.
         *
         * @returns Size of the value table.
         *
         * @pre @code version() < 3 @endcode
         */
        std::size_t value_table_size() const noexcept {
            vtzero_assert_in_noexcept_function(version() < 3);
            return m_value_table_size > 0 ? m_value_table_size : m_value_table.size();
        }

        /**
         * Get the property key with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws out_of_range_exception if the index is out of range.
         * @pre @code valid() @endcode
         */
        data_view key(index_value index) const {
            vtzero_assert(valid());

            const auto& table = key_table();
            if (index.value() >= table.size()) {
                throw out_of_range_exception{index.value(), m_layer_num};
            }

            return table[index.value()];
        }

        /**
         * Get the property value with the given index.
         *
         * Complexity: Amortized constant. First time the table is needed
         *             it needs to be created.
         *
         * @throws out_of_range_exception if the index is out of range.
         * @pre @code valid() @endcode
         * @pre @code version() < 3 @endcode
         */
        property_value value(index_value index) const {
            vtzero_assert(valid());
            vtzero_assert(version() < 3);

            const auto& table = value_table();
            if (index.value() >= table.size()) {
                throw out_of_range_exception{index.value(), m_layer_num};
            }

            return table[index.value()];
        }

        /**
         * Get the feature with the specified ID. If there are several features
         * with the same ID, it is undefined which one you'll get.
         *
         * Note that the feature returned will internally contain a pointer to
         * the layer it came from. The layer has to stay valid as long as the
         * feature is used.
         *
         * Complexity: Linear in the number of features.
         *
         * @param id The ID to look for.
         * @returns Feature with the specified ID or the invalid feature if
         *          there is no feature with this ID.
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        feature get_feature_by_id(uint64_t id) const {
            vtzero_assert(valid());

            std::size_t feature_num = 0;
            protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
            while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                const auto feature_data = layer_reader.get_view();
                protozero::pbf_message<detail::pbf_feature> feature_reader{feature_data};
                if (feature_reader.next(detail::pbf_feature::id, protozero::pbf_wire_type::varint)) {
                    if (feature_reader.get_uint64() == id) {
                        return feature{this, feature_data, feature_num};
                    }
                }
                ++feature_num;
            }

            return feature{};
        }

        /**
         * Get the elevation scaling.
         */
        const scaling& elevation_scaling() const noexcept {
            return m_elevation_scaling;
        }

        /**
         * Get the attribute scaling with the specified index.
         *
         * @param index The index of the scaling requested.
         * @returns scaling
         * @throws std::out_of_range if the index is out of range.
         */
        const scaling& attribute_scaling(index_value index) const {
            return m_attribute_scalings.at(index.value());
        }

        /**
         * Return the number of attribute scalings defined in this layer.
         */
        std::size_t num_attribute_scalings() const noexcept {
            return m_attribute_scalings.size();
        }

        /// Get a (const) iterator to the first feature in this layer.
        feature_iterator begin() const noexcept {
            vtzero_assert_in_noexcept_function(m_data.data());
            return {m_data, this};
        }

        /// Get a (const) iterator one past the end feature in this layer.
        feature_iterator end() const noexcept {
            vtzero_assert_in_noexcept_function(m_data.data());
            return {data_view{m_data.data() + m_data.size(), 0}, this};
        }

    }; // class layer

    inline std::size_t feature::layer_num() const noexcept {
        return m_layer->layer_num();
    }

    inline feature feature_iterator::operator*() const {
        protozero::pbf_message<detail::pbf_layer> reader{m_data};
        if (reader.next(detail::pbf_layer::features,
                        protozero::pbf_wire_type::length_delimited)) {
            return {m_layer, reader.get_view(), m_feature_num};
        }

        // abort if caller tried to dereference an invalid iterator
        std::abort();
    }

    inline feature::feature(const layer* layer, const data_view data, std::size_t feature_num) :
        m_layer(layer),
        m_feature_num(feature_num) {
        vtzero_assert(layer);
        vtzero_assert(data.data());

        protozero::pbf_message<detail::pbf_feature> reader{data};

        while (reader.next()) {
            switch (reader.tag()) {
                case detail::pbf_feature::id:
                    if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                        throw format_exception{"Feature id has wrong protobuf type", layer_num(), feature_num};
                    }
                    if (m_id_type != id_type::no_id) {
                        throw format_exception{"Feature has more than one id/string_id", layer_num(), feature_num};
                    }
                    m_integer_id = reader.get_uint64();
                    m_id_type = id_type::integer_id;
                    break;
                case detail::pbf_feature::tags:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature tags have wrong protobuf type", layer_num(), feature_num};
                    }
                    if (!m_tags.empty()) {
                        throw format_exception{"Feature has more than one tags field", layer_num(), feature_num};
                    }
                    if (!m_attributes.empty()) {
                        throw format_exception{"Feature has both tags and attributes field (spec 4.4)", layer_num(), feature_num};
                    }
                    m_tags = reader.get_view();
                    break;
                case detail::pbf_feature::attributes:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature attributes have wrong protobuf type", layer_num(), feature_num};
                    }
                    if (layer->version() <= 2) {
                        throw format_exception{"Attributes in feature in layer with version <= 2", layer_num(), feature_num};
                    }
                    if (!m_attributes.empty()) {
                        throw format_exception{"Feature has more than one attributes field", layer_num(), feature_num};
                    }
                    if (!m_tags.empty()) {
                        throw format_exception{"Feature has both tags and attributes field (spec 4.4)", layer_num(), feature_num};
                    }
                    m_attributes = reader.get_view();
                    break;
                case detail::pbf_feature::type: {
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Feature type has wrong protobuf type", layer_num(), feature_num};
                        }
                        const int32_t type = reader.get_enum();
                        // spec 4.3.4 "Geometry Types"
                        if (type < 0 || type > static_cast<int32_t>(GeomType::max)) {
                            throw format_exception{"Unknown geometry type in feature (spec 4.3.5)", layer_num(), feature_num};
                        }
                        m_geometry_type = static_cast<GeomType>(type);
                    }
                    break;
                case detail::pbf_feature::geometry:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature geometry has wrong protobuf type", layer_num(), feature_num};
                    }
                    if (!m_geometry.empty()) {
                        throw format_exception{"Feature has more than one geometry", layer_num(), feature_num};
                    }
                    m_geometry = reader.get_view();
                    break;
                case detail::pbf_feature::elevations:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature elevations have wrong protobuf type", layer_num(), feature_num};
                    }
                    if (layer->version() <= 2) {
                        throw format_exception{"Elevation in feature in layer with version <= 2", layer_num(), feature_num};
                    }
                    if (!m_elevations.empty()) {
                        throw format_exception{"Feature has more than one elevations field", layer_num(), feature_num};
                    }
                    m_elevations = reader.get_view();
                    break;
                case detail::pbf_feature::geometric_attributes:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature geometric_attributes wrong protobuf type", layer_num(), feature_num};
                    }
                    if (layer->version() <= 2) {
                        throw format_exception{"Geometric attribute in feature in layer with version <= 2", layer_num(), feature_num};
                    }
                    if (!m_geometric_attributes.empty()) {
                        throw format_exception{"Feature has more than one geometric attributes field", layer_num(), feature_num};
                    }
                    m_geometric_attributes = reader.get_view();
                    break;
                case detail::pbf_feature::string_id:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature string_id has wrong protobuf type", layer_num(), feature_num};
                    }
                    if (layer->version() <= 2) {
                        throw format_exception{"String id in feature in layer with version <= 2", layer_num(), feature_num};
                    }
                    if (m_id_type != id_type::no_id) {
                        throw format_exception{"Feature has more than one id/string_id", layer_num(), feature_num};
                    }
                    m_string_id = reader.get_view();
                    m_id_type = id_type::string_id;
                    break;
                case detail::pbf_feature::spline_knots:
                    if (reader.wire_type() != protozero::pbf_wire_type::length_delimited) {
                        throw format_exception{"Feature spline_knots have wrong protobuf type", layer_num(), feature_num};
                    }
                    if (!m_knots.empty()) {
                        throw format_exception{"Feature has more than one spline knots field", layer_num(), feature_num};
                    }
                    m_knots = reader.get_view();
                    break;
                case detail::pbf_feature::spline_degree:
                    if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                        throw format_exception{"Feature spline_degree has wrong protobuf type", layer_num(), feature_num};
                    }
                    m_spline_degree = reader.get_uint32();
                    if (m_spline_degree < 2 || m_spline_degree > 3) {
                        throw format_exception{"Spline degree in feature must be 2 or 3", layer_num(), feature_num};
                    }
                    break;
                default:
                    reader.skip(); // ignore unknown fields
            }
        }

        // spec 4.2 "A feature MUST contain a geometry field."
        if (m_geometry.empty()) {
            throw format_exception{"Missing geometry field in feature (spec 4.3)", layer_num(), feature_num};
        }
    }

    namespace detail {

        template <typename THandler, typename TIterator>
        bool decode_attribute(THandler&& handler, const feature& feature, std::size_t depth, TIterator& it, TIterator end);

        namespace lookup {

            // These functions are used to look up a value in one of the
            // tables in the layers.

            inline data_view key_index(const layer& layer, index_value idx) {
                return layer.key(idx);
            }

            inline data_view value_index_string(const layer& layer, index_value idx) {
                return layer.value(idx).string_value();
            }

            inline float value_index_float(const layer& layer, index_value idx) {
                return layer.value(idx).float_value();
            }

            inline double value_index_double(const layer& layer, index_value idx) {
                return layer.value(idx).double_value();
            }

            inline int64_t value_index_int(const layer& layer, index_value idx) {
                return layer.value(idx).int_value();
            }

            inline uint64_t value_index_uint(const layer& layer, index_value idx) {
                return layer.value(idx).uint_value();
            }

            inline int64_t value_index_sint(const layer& layer, index_value idx) {
                return layer.value(idx).sint_value();
            }

            inline bool value_index_bool(const layer& layer, index_value idx) {
                return layer.value(idx).bool_value();
            }

            inline data_view string_index(const layer& layer, index_value idx) {
                return layer.string_table().at(idx.value());
            }

            inline float float_index(const layer& layer, index_value idx) {
                return layer.float_table().at(idx);
            }

            inline double double_index(const layer& layer, index_value idx) {
                return layer.double_table().at(idx);
            }

            inline int64_t int_index(const layer& layer, index_value idx) {
                return protozero::decode_zigzag64(layer.int_table().at(idx));
            }

            inline uint64_t uint_index(const layer& layer, index_value idx) {
                return layer.int_table().at(idx);
            }

        } // namespace lookup

        /**
         * Call a function after doing a lookup on some index in a layer
         * using the functions from the "lookup" namespace.
         */
#define DEF_CALL_WITH_LAYER_WRAPPER(func, ptype) \
        template <typename THandler, typename TLookupFunc, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<ptype>(), std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func##_##ptype(THandler&& handler, TLookupFunc&& lookup, const layer& layer, index_value idx, std::size_t depth) { \
            return std::forward<THandler>(handler).func(std::forward<TLookupFunc>(lookup)(layer, idx), depth); \
        } \
        template <typename THandler, typename TLookupFunc, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<ptype>(), std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func##_##ptype(THandler&& handler, TLookupFunc&& lookup, const layer& layer, index_value idx, std::size_t depth) { \
            std::forward<THandler>(handler).func(std::forward<TLookupFunc>(lookup)(layer, idx), depth); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func##_##ptype(TArgs&&... /*unused*/) { \
            return true; \
        } \

        DEF_CALL_WITH_LAYER_WRAPPER(attribute_key, data_view)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, double)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, float)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, data_view)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, int64_t)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, uint64_t)
        DEF_CALL_WITH_LAYER_WRAPPER(attribute_value, bool)

#undef DEF_CALL_WITH_LAYER_WRAPPER

        template <typename THandler, typename TIterator>
        bool decode_structured_value(THandler&& handler, const feature& feature, std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw format_exception{"Attributes list is missing value", feature.layer_num(), feature.feature_num()};
            }

            const uint64_t structured_value = *it++;

            const auto vt = structured_value & 0x0fu;
            if (vt > static_cast<std::size_t>(structured_value_type::max)) {
                throw format_exception{"Unknown structured value type: " + std::to_string(vt), feature.layer_num(), feature.feature_num()};
            }

            auto vp = structured_value >> 4u;
            switch (static_cast<structured_value_type>(vt)) {
                case structured_value_type::cvt_inline_sint:
                    if (!call_attribute_value<THandler>(std::forward<THandler>(handler), protozero::decode_zigzag64(vp), depth)) {
                        return false;
                    }
                    break;
                case structured_value_type::cvt_inline_uint:
                    if (!call_attribute_value<THandler>(std::forward<THandler>(handler), vp, depth)) {
                        return false;
                    }
                    break;
                case structured_value_type::cvt_bool:
                    switch (vp) {
                        case 0:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), null_type{}, depth)) {
                                return false;
                            }
                            break;
                        case 1:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), false, depth)) {
                                return false;
                            }
                            break;
                        case 2:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), true, depth)) {
                                return false;
                            }
                            break;
                        default:
                            throw format_exception{"Invalid value for bool/null structured value: " + std::to_string(vp), feature.layer_num(), feature.feature_num()};
                    }
                    break;
                case structured_value_type::cvt_double:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_double_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_double<THandler>(std::forward<THandler>(handler), lookup::double_index, feature.get_layer(), idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case structured_value_type::cvt_float:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_float_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_float<THandler>(std::forward<THandler>(handler), lookup::float_index, feature.get_layer(), idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case structured_value_type::cvt_string:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_string_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_data_view<THandler>(std::forward<THandler>(handler), lookup::string_index, feature.get_layer(), idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case structured_value_type::cvt_sint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), lookup::int_index, feature.get_layer(), idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case structured_value_type::cvt_uint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_uint64_t<THandler>(std::forward<THandler>(handler), lookup::uint_index, feature.get_layer(), idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case structured_value_type::cvt_list:
                    if (!call_start_list_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_structured_value(handler, feature, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!call_end_list_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                case structured_value_type::cvt_map:
                    if (!call_start_map_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_attribute(handler, feature, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!call_end_map_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                case structured_value_type::cvt_number_list: {
                    index_value index{static_cast<uint32_t>(*it++)};
                    if (it == end) {
                        throw format_exception{"Attributes list is missing value", feature.layer_num(), feature.feature_num()};
                    }
                    if (!call_start_number_list<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), index, depth)) {
                        return false;
                    }
                    int64_t value = 0;
                    while (vp > 0) {
                        if (it == end) {
                            throw format_exception{"Attributes list is missing value", feature.layer_num(), feature.feature_num()};
                        }
                        --vp;
                        const auto encoded_value = *it++;
                        if (encoded_value == 0) { // null
                            if (!call_number_list_null_value<THandler>(std::forward<THandler>(handler), depth)) {
                                return false;
                            }
                        } else {
                            value += protozero::decode_zigzag64(encoded_value - 1);
                            if (!call_number_list_value<THandler>(std::forward<THandler>(handler), value, depth)) {
                                return false;
                            }
                        }
                    }
                    if (!call_end_number_list<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                }
            }

            return true;
        }

        template <typename THandler, typename TIterator>
        bool decode_attribute(THandler&& handler, const feature& feature, std::size_t depth, TIterator& it, TIterator end) {
            const auto ki = static_cast<uint32_t>(*it++);
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki, feature.layer_num()};
            }

            if (!call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, depth)) {
                skip_structured_value(&feature, depth, it, end);
                return true;
            }

            if (!call_attribute_key_data_view<THandler>(std::forward<THandler>(handler), lookup::key_index, feature.get_layer(), index_value{ki}, depth)) {
                skip_structured_value(&feature, depth, it, end);
                return true;
            }

            return decode_structured_value(handler, feature, depth, it, end);
        }

    } // namespace detail

    template <typename THandler>
    void feature::decode_tags_impl(THandler&& handler) const {
        const std::size_t depth = 0;
        const auto end = m_tags.it_end();
        for (auto it = m_tags.it_begin(); it != end;) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki, layer_num()};
            }

            if (it == end) {
                throw format_exception{"Unpaired attributes key/value indexes (spec 4.4)", layer_num(), m_feature_num};
            }
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi, layer_num()};
            }

            if (!detail::call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, depth)) {
                continue;
            }
            if (!detail::call_attribute_key_data_view<THandler>(std::forward<THandler>(handler), detail::lookup::key_index, *m_layer, index_value{ki}, depth)) {
                continue;
            }
            if (!detail::call_value_index<THandler>(std::forward<THandler>(handler), index_value{vi}, depth)) {
                return;
            }

            switch (m_layer->value(vi).type()) {
                case property_value_type::string_value:
                    if (!detail::call_attribute_value_data_view<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_string, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::float_value:
                    if (!detail::call_attribute_value_float<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_float, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::double_value:
                    if (!detail::call_attribute_value_double<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_double, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::int_value:
                    if (!detail::call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_int, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::uint_value:
                    if (!detail::call_attribute_value_uint64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_uint, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::sint_value:
                    if (!detail::call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_sint, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
                case property_value_type::bool_value:
                    if (!detail::call_attribute_value_bool<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_bool, *m_layer, index_value{vi}, depth)) {
                        return;
                    }
                    break;
            }
        }
    }

    template <typename THandler>
    bool feature::decode_attributes_impl(THandler&& handler) const {
        const auto end = m_attributes.it_end();
        for (auto it = m_attributes.it_begin(); it != end;) {
            if (!detail::decode_attribute(std::forward<THandler>(handler), *this, 0, it, end)) {
                return false;
            }
        }
        return true;
    }

    template <typename THandler>
    detail::get_result_t<THandler> feature::decode_attributes(THandler&& handler) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer != nullptr);

        if (!tags_data().empty()) {
            decode_tags_impl(std::forward<THandler>(handler));
        }

        if (!attributes_data().empty()) {
            decode_attributes_impl(std::forward<THandler>(handler));
        }

        return detail::get_result<THandler>::of(std::forward<THandler>(handler));
    }

    template <typename THandler>
    bool feature::decode_geometric_attributes_impl(THandler&& handler) const {
        const auto end = m_geometric_attributes.it_end();
        for (auto it = m_geometric_attributes.it_begin(); it != end;) {
            if (!detail::decode_attribute(std::forward<THandler>(handler), *this, 0, it, end)) {
                return false;
            }
        }
        return true;
    }

    template <typename THandler>
    detail::get_result_t<THandler> feature::decode_geometric_attributes(THandler&& handler) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer != nullptr);
        vtzero_assert(m_layer->version() == 3);

        decode_geometric_attributes_impl(std::forward<THandler>(handler));

        return detail::get_result<THandler>::of(std::forward<THandler>(handler));
    }

    template <typename THandler>
    detail::get_result_t<THandler> feature::decode_all_attributes(THandler&& handler) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer != nullptr);
        vtzero_assert(m_layer->version() == 3);

        if (decode_attributes_impl(std::forward<THandler>(handler))) {
            decode_geometric_attributes_impl(std::forward<THandler>(handler));
        }

        return detail::get_result<THandler>::of(std::forward<THandler>(handler));
    }

} // namespace vtzero

#endif // VTZERO_LAYER_HPP
