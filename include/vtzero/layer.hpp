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

#include "attributes.hpp"
#include "exception.hpp"
#include "feature.hpp"
#include "geometry.hpp"
#include "property_value.hpp"
#include "scaling.hpp"
#include "tile.hpp"
#include "types.hpp"
#include "unaligned_table.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <iterator>
#include <vector>

namespace vtzero {

    /**
     * For iterating over all features in a vector tile layer.
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

        using iterator_category = std::forward_iterator_tag;
        using value_type        = feature;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        feature_iterator() noexcept = default;

        feature_iterator(data_view data, const layer* layer) noexcept :
            m_data(data),
            m_layer(layer) {
            skip_non_features();
        }

        value_type operator*() const {
            protozero::pbf_message<detail::pbf_layer> reader{m_data};
            if (reader.next(detail::pbf_layer::features,
                            protozero::pbf_wire_type::length_delimited)) {
                return {m_layer, reader.get_view()};
            }
            throw format_exception{"expected feature"};
        }

        feature_iterator& operator++() {
            if (!m_data.empty()) {
                protozero::pbf_message<detail::pbf_layer> reader{m_data};
                if (reader.next(detail::pbf_layer::features,
                                protozero::pbf_wire_type::length_delimited)) {
                    reader.skip();
                    m_data = reader.data();
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

        friend bool operator==(feature_iterator lhs, feature_iterator rhs) noexcept {
            return lhs.m_data == rhs.m_data;
        }

        friend bool operator!=(feature_iterator lhs, feature_iterator rhs) noexcept {
            return !(lhs == rhs);
        }

    }; // class feature_iterator

    /**
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
        std::size_t m_num_features = 0;
        data_view m_name{};
        protozero::pbf_message<detail::pbf_layer> m_layer_reader{m_data};

        // Elevation scaling
        scaling m_elevation_scaling{};

        // Geometric attribute scalings
        std::vector<scaling> m_attribute_scalings{};

        mutable std::vector<data_view> m_key_table;
        mutable std::vector<property_value> m_value_table;
        mutable std::size_t m_key_table_size = 0;
        mutable std::size_t m_value_table_size = 0;

        mutable std::vector<data_view> m_string_table;
        mutable std::size_t m_string_table_size = 0;

        detail::unaligned_table<double> m_double_table;
        detail::unaligned_table<float> m_float_table;
        detail::unaligned_table<uint64_t> m_int_table;

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
         * user code, because layers are created by the tile_iterator.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 to 3 are supported)
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        explicit layer(const data_view data) :
            m_data(data) {
            protozero::pbf_message<detail::pbf_layer> reader{data};
            uint32_t x = 0;
            uint32_t y = 0;
            uint32_t zoom = 0;
            bool has_x_y_zoom = false;
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
                    case protozero::tag_and_type(detail::pbf_layer::string_values, protozero::pbf_wire_type::length_delimited):
                        reader.skip();
                        ++m_string_table_size;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::double_values, protozero::pbf_wire_type::length_delimited):
                        if (!m_double_table.empty()) {
                            throw format_exception{"more than one double_table in the layer"};
                        }
                        m_double_table = detail::unaligned_table<double>{reader.get_view()};
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::float_values, protozero::pbf_wire_type::length_delimited):
                        if (!m_float_table.empty()) {
                            throw format_exception{"more than one float_table in the layer"};
                        }
                        m_float_table = detail::unaligned_table<float>{reader.get_view()};
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::int_values, protozero::pbf_wire_type::length_delimited):
                        if (!m_int_table.empty()) {
                            throw format_exception{"more than one int_table in the layer"};
                        }
                        m_int_table = detail::unaligned_table<uint64_t>{reader.get_view()};
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::elevation_scaling, protozero::pbf_wire_type::length_delimited):
                        m_elevation_scaling = scaling{reader.get_view()};
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::attribute_scalings, protozero::pbf_wire_type::length_delimited):
                        m_attribute_scalings.emplace_back(reader.get_view());
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::tile_x, protozero::pbf_wire_type::varint):
                        x = reader.get_uint32();
                        has_x_y_zoom = true;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::tile_y, protozero::pbf_wire_type::varint):
                        y = reader.get_uint32();
                        has_x_y_zoom = true;
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::tile_zoom, protozero::pbf_wire_type::varint):
                        zoom = reader.get_uint32();
                        if (zoom >= tile::max_zoom) {
                            throw format_exception{"zoom level too large"};
                        }
                        has_x_y_zoom = true;
                        break;
                    default:
                        throw format_exception{"unknown field in layer (tag=" +
                                               std::to_string(static_cast<uint32_t>(reader.tag())) +
                                               ", type=" +
                                               std::to_string(static_cast<uint32_t>(reader.wire_type())) +
                                               ")"};
                }
            }

            // This library can only handle version 1 to 3.
            if (m_version < 1 || m_version > 3) {
                throw version_exception{m_version};
            }

            // Check for vt3 components in vt2 layers.
            if (m_version <= 2) {
                if (m_string_table_size > 0) {
                    throw format_exception{"found entry in string table in layer with version <= 2"};
                }
                if (!m_double_table.empty()) {
                    throw format_exception{"found entry in double table in layer with version <= 2"};
                }
                if (!m_float_table.empty()) {
                    throw format_exception{"found entry in float table in layer with version <= 2"};
                }
                if (!m_int_table.empty()) {
                    throw format_exception{"found entry in int table in layer with version <= 2"};
                }
                if (m_elevation_scaling != scaling{}) {
                    throw format_exception{"found elevation scaling message in layer with version <= 2"};
                }
                if (!m_attribute_scalings.empty()) {
                    throw format_exception{"found attribute scaling message in layer with version <= 2"};
                }
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.data() == nullptr) {
                throw format_exception{"missing name field in layer (spec 4.1)"};
            }

            if (has_x_y_zoom) {
                if (x >= detail::num_tiles_in_zoom(zoom)) {
                    throw format_exception{"tile x value out of range"};
                }

                if (y >= detail::num_tiles_in_zoom(zoom)) {
                    throw format_exception{"tile y value out of range"};
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
         * @pre @code version() < 3 @endcode
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
        const detail::unaligned_table<double>& double_table() const noexcept {
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
        const detail::unaligned_table<float>& float_table() const noexcept {
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
        const detail::unaligned_table<uint64_t>& int_table() const noexcept {
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
                throw out_of_range_exception{index.value()};
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
                throw out_of_range_exception{index.value()};
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

    inline feature::feature(const layer* layer, const data_view data) :
        m_layer(layer) {
        vtzero_assert(layer);
        vtzero_assert(data.data());

        protozero::pbf_message<detail::pbf_feature> reader{data};

        while (reader.next()) {
            switch (reader.tag_and_type()) {
                case protozero::tag_and_type(detail::pbf_feature::id, protozero::pbf_wire_type::varint):
                    if (m_id_type != id_type::no_id) {
                        throw format_exception{"Feature has more than one id/string_id field"};
                    }
                    m_integer_id = reader.get_uint64();
                    m_id_type = id_type::integer_id;
                    break;
                case protozero::tag_and_type(detail::pbf_feature::tags, protozero::pbf_wire_type::length_delimited):
                    if (!m_tags.empty()) {
                        throw format_exception{"Feature has more than one tags field"};
                    }
                    if (!m_attributes.empty()) {
                        throw format_exception{"Feature has both tags and attributes field"};
                    }
                    m_tags = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::attributes, protozero::pbf_wire_type::length_delimited):
                    if (layer->version() <= 2) {
                        throw format_exception{"Found attributes field in layer with version <= 2"};
                    }
                    if (!m_attributes.empty()) {
                        throw format_exception{"Feature has more than one attributes field"};
                    }
                    if (!m_tags.empty()) {
                        throw format_exception{"Feature has both tags and attributes field"};
                    }
                    m_attributes = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::type, protozero::pbf_wire_type::varint): {
                        const int32_t type = reader.get_enum();
                        // spec 4.3.4 "Geometry Types"
                        if (type < 0 || type > static_cast<int32_t>(GeomType::max)) {
                            throw format_exception{"Unknown geometry type (spec 4.3.4)"};
                        }
                        m_geometry_type = static_cast<GeomType>(type);
                    }
                    break;
                case protozero::tag_and_type(detail::pbf_feature::geometry, protozero::pbf_wire_type::length_delimited):
                    if (!m_geometry.empty()) {
                        throw format_exception{"Feature has more than one geometry field"};
                    }
                    m_geometry = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::elevations, protozero::pbf_wire_type::length_delimited):
                    if (layer->version() <= 2) {
                        throw format_exception{"Found elevation in layer with version <= 2"};
                    }
                    if (!m_elevations.empty()) {
                        throw format_exception{"Feature has more than one elevations field"};
                    }
                    m_elevations = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::geometric_attributes, protozero::pbf_wire_type::length_delimited):
                    if (layer->version() <= 2) {
                        throw format_exception{"Found geometric attribute in layer with version <= 2"};
                    }
                    if (!m_geometric_attributes.empty()) {
                        throw format_exception{"Feature has more than one geometric_attributes field"};
                    }
                    m_geometric_attributes = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::string_id, protozero::pbf_wire_type::length_delimited):
                    if (layer->version() <= 2) {
                        throw format_exception{"Found String ID in layer with version <= 2"};
                    }
                    if (m_id_type != id_type::no_id) {
                        throw format_exception{"Feature has more than one id/string_id field"};
                    }
                    m_string_id = reader.get_view();
                    m_id_type = id_type::string_id;
                    break;
                case protozero::tag_and_type(detail::pbf_feature::spline_knots, protozero::pbf_wire_type::length_delimited):
                    if (!m_knots.empty()) {
                        throw format_exception{"Feature has more than one spline_knots field"};
                    }
                    m_knots = reader.get_view();
                    break;
                case protozero::tag_and_type(detail::pbf_feature::spline_degree, protozero::pbf_wire_type::varint):
                    m_spline_degree = reader.get_uint32();
                    if (m_spline_degree < 2 || m_spline_degree > 3) {
                        throw format_exception{"Spline degree must be 2 or 3"};
                    }
                    break;
                default:
                    reader.skip(); // ignore unknown fields
            }
        }

        // spec 4.2 "A feature MUST contain a geometry field."
        if (m_geometry.empty()) {
            throw format_exception{"Missing geometry field in feature (spec 4.2)"};
        }
    }

    namespace detail {

        template <typename THandler, typename TIterator>
        bool decode_attribute(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end);

        template <typename TIterator>
        void skip_complex_value(std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw format_exception{"Attributes list is missing value"};
            }

            const uint64_t complex_value = *it++;

            const auto vt = complex_value & 0x0fu;
            if (vt > static_cast<std::size_t>(complex_value_type::max)) {
                throw format_exception{"unknown complex value type: " + std::to_string(vt)};
            }

            const auto cvt = static_cast<complex_value_type>(vt);

            if (cvt == complex_value_type::cvt_list) {
                auto vp = complex_value >> 4u;
                while (vp > 0) {
                    --vp;
                    skip_complex_value(depth + 1, it, end);
                }
            } else if (cvt == complex_value_type::cvt_map) {
                auto vp = complex_value >> 4u;
                while (vp > 0) {
                    --vp;
                    ++it; // skip key
                    if (it == end) {
                        throw format_exception{"Attributes map is missing value"};
                    }
                    skip_complex_value(depth + 1, it, end);
                }
            }
        }

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
        bool decode_complex_value(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw format_exception{"Attributes list is missing value"};
            }

            const uint64_t complex_value = *it++;

            const auto vt = complex_value & 0x0fu;
            if (vt > static_cast<std::size_t>(complex_value_type::max)) {
                throw format_exception{"unknown complex value type: " + std::to_string(vt)};
            }

            auto vp = complex_value >> 4u;
            switch (static_cast<complex_value_type>(vt)) {
                case complex_value_type::cvt_inline_sint:
                    if (!call_attribute_value<THandler>(std::forward<THandler>(handler), protozero::decode_zigzag64(vp), depth)) {
                        return false;
                    }
                    break;
                case complex_value_type::cvt_inline_uint:
                    if (!call_attribute_value<THandler>(std::forward<THandler>(handler), vp, depth)) {
                        return false;
                    }
                    break;
                case complex_value_type::cvt_bool:
                    switch (vp) {
                        case 0:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), false, depth)) {
                                return false;
                            }
                            break;
                        case 1:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), true, depth)) {
                                return false;
                            }
                            break;
                        case 2:
                            if (!call_attribute_value<THandler>(std::forward<THandler>(handler), null_type{}, depth)) {
                                return false;
                            }
                            break;
                        default:
                            throw format_exception{"invalid value for bool/null value: " + std::to_string(vp)};
                    }
                    break;
                case complex_value_type::cvt_double:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_double_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_double<THandler>(std::forward<THandler>(handler), lookup::double_index, layer, idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case complex_value_type::cvt_float:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_float_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_float<THandler>(std::forward<THandler>(handler), lookup::float_index, layer, idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case complex_value_type::cvt_string:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_string_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_data_view<THandler>(std::forward<THandler>(handler), lookup::string_index, layer, idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case complex_value_type::cvt_sint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), lookup::int_index, layer, idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case complex_value_type::cvt_uint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!call_attribute_value_uint64_t<THandler>(std::forward<THandler>(handler), lookup::uint_index, layer, idx, depth)) {
                            return false;
                        }
                    }
                    break;
                case complex_value_type::cvt_list:
                    if (!call_start_list_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_complex_value(handler, layer, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!call_end_list_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                case complex_value_type::cvt_map:
                    if (!call_start_map_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_attribute(handler, layer, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!call_end_map_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                case complex_value_type::cvt_number_list: {
                    index_value index{static_cast<uint32_t>(*it++)};
                    if (it == end) {
                        throw format_exception{"Attributes list is missing value"};
                    }
                    if (!call_start_number_list<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), index, depth)) {
                        return false;
                    }
                    int64_t value = 0;
                    while (vp > 0) {
                        if (it == end) {
                            throw format_exception{"Attributes list is missing value"};
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
        bool decode_attribute(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end) {
            const auto ki = static_cast<uint32_t>(*it++);
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            if (!call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, depth)) {
                skip_complex_value(depth, it, end);
                return true;
            }

            if (!call_attribute_key_data_view<THandler>(std::forward<THandler>(handler), lookup::key_index, layer, index_value{ki}, depth)) {
                skip_complex_value(depth, it, end);
                return true;
            }

            return decode_complex_value(handler, layer, depth, it, end);
        }

    } // namespace detail

    template <typename THandler>
    void feature::decode_tags_impl(THandler&& handler) const {
        for (auto it = tags_begin(); it != tags_end();) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            if (it == tags_end()) {
                throw format_exception{"unpaired attributes key/value indexes (spec 4.4)"};
            }
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi};
            }

            if (!detail::call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, static_cast<std::size_t>(0))) {
                continue;
            }
            if (!detail::call_attribute_key_data_view<THandler>(std::forward<THandler>(handler), detail::lookup::key_index, *m_layer, index_value{ki}, static_cast<std::size_t>(0))) {
                continue;
            }
            if (!detail::call_value_index<THandler>(std::forward<THandler>(handler), index_value{vi}, static_cast<std::size_t>(0))) {
                return;
            }

            switch (m_layer->value(vi).type()) {
                case property_value_type::string_value:
                    if (!detail::call_attribute_value_data_view<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_string, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::float_value:
                    if (!detail::call_attribute_value_float<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_float, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::double_value:
                    if (!detail::call_attribute_value_double<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_double, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::int_value:
                    if (!detail::call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_int, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::uint_value:
                    if (!detail::call_attribute_value_uint64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_uint, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::sint_value:
                    if (!detail::call_attribute_value_int64_t<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_sint, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
                case property_value_type::bool_value:
                    if (!detail::call_attribute_value_bool<THandler>(std::forward<THandler>(handler), detail::lookup::value_index_bool, *m_layer, index_value{vi}, static_cast<std::size_t>(0))) {
                        return;
                    }
                    break;
            }
        }
    }

    template <typename THandler>
    bool feature::decode_attributes_impl(THandler&& handler) const {
        const auto end = attributes_end();
        for (auto it = attributes_begin(); it != end;) {
            if (!detail::decode_attribute(std::forward<THandler>(handler), *m_layer, 0, it, end)) {
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
        const auto end = geometric_attributes_end();
        for (auto it = geometric_attributes_begin(); it != end;) {
            if (!detail::decode_attribute(std::forward<THandler>(handler), *m_layer, 0, it, end)) {
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
