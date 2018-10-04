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
#include "types.hpp"
#include "unaligned_table.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <iterator>
#include <vector>

namespace vtzero {

    /**
     * A layer according to spec 4.1. It contains a version, the extent,
     * and a name. For the most efficient way to access the features in this
     * layer call next_feature() until it returns an invalid feature:
     *
     * @code
     *   std::string data = ...;
     *   vector_tile tile{data};
     *   layer = tile.next_layer();
     *   while (auto feature = layer.next_feature()) {
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
        uint32_t m_version = 1; // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
        uint32_t m_extent = 4096; // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70
        std::size_t m_num_features = 0;
        data_view m_name{};
        protozero::pbf_message<detail::pbf_layer> m_layer_reader{m_data};

        std::vector<scaling> m_scalings = {scaling{}};

        mutable std::vector<data_view> m_key_table;
        mutable std::vector<property_value> m_value_table;
        mutable std::size_t m_key_table_size = 0;
        mutable std::size_t m_value_table_size = 0;

        mutable std::vector<data_view> m_string_table;
        mutable std::size_t m_string_table_size = 0;

        detail::unaligned_table<double> m_double_table;
        detail::unaligned_table<float> m_float_table;
        detail::unaligned_table<uint64_t> m_int_table;

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
                        m_scalings[0] = scaling{reader.get_view()};
                        break;
                    case protozero::tag_and_type(detail::pbf_layer::attribute_scalings, protozero::pbf_wire_type::length_delimited):
                        m_scalings.emplace_back(reader.get_view());
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
            vtzero_assert(version() < 3);

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
         * Get the next feature in this layer.
         *
         * Note that the feature returned will internally contain a pointer to
         * the layer it came from. The layer has to stay valid as long as the
         * feature is used.
         *
         * Complexity: Constant.
         *
         * @returns The next feature or the invalid feature if there are no
         *          more features.
         * @throws format_exception if the layer data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        feature next_feature() {
            vtzero_assert(valid());

            const bool has_next = m_layer_reader.next(detail::pbf_layer::features,
                                                      protozero::pbf_wire_type::length_delimited);

            return has_next ? feature{this, m_layer_reader.get_view()} : feature{};
        }

        /**
         * Reset the feature iterator. The next time next_feature() is called,
         * it will begin from the first feature again.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        void reset_feature() noexcept {
            vtzero_assert_in_noexcept_function(valid());

            m_layer_reader = protozero::pbf_message<detail::pbf_layer>{m_data};
        }

        /**
         * Call a function for each feature in this layer.
         *
         * @tparam The type of the function. It must take a single argument
         *         of type feature&& and return a bool. If the function returns
         *         false, the iteration will be stopped.
         * @param func The function to call.
         * @returns true if the iteration was completed and false otherwise.
         * @pre @code valid() @endcode
         */
        template <typename TFunc>
        bool for_each_feature(TFunc&& func) const {
            vtzero_assert(valid());

            protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
            while (layer_reader.next(detail::pbf_layer::features,
                                     protozero::pbf_wire_type::length_delimited)) {
                if (!std::forward<TFunc>(func)(feature{this, layer_reader.get_view()})) {
                    return false;
                }
            }

            return true;
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
         * Scale the given elevation from a vector tile according to the
         * scaling stored in the layer.
         *
         * @param elevation The elevation of a point.
         * @returns Scaled elevation.
         */
        double scale_elevation(int64_t elevation) const noexcept {
            vtzero_assert(!m_scalings.empty());
            return m_scalings[0].decode(elevation);
        }

        /**
         * Scale the given geometric attribute value according to the
         * scaling stored in the layer.
         *
         * @param index The scaling configuration to be used.
         * @param value The value of a geometric attribute at some point.
         * @returns Scaled value.
         */
        double scale_attribute_value(std::size_t index, int64_t value) const {
            if (index + 1 >= m_scalings.size()) {
                throw format_exception{"non-existent scaling index: " + std::to_string(index)};
            }
            return m_scalings[index + 1].decode(value);
        }

    }; // class layer

    inline property feature::next_property() {
        vtzero_assert(m_layer->version() < 3);
        const auto idxs = next_property_indexes();
        property p{};
        if (idxs.valid()) {
            p = {m_layer->key(idxs.key()),
                 m_layer->value(idxs.value())};
        }
        return p;
    }

    inline index_value_pair feature::next_property_indexes() {
        vtzero_assert(valid());
        vtzero_assert(m_layer->version() < 3);

        if (m_property_iterator == m_properties.end()) {
            return {};
        }

        const auto ki = *m_property_iterator++;
        if (!index_value{ki}.valid()) {
            throw out_of_range_exception{ki};
        }

        vtzero_assert(m_property_iterator != m_properties.end());
        const auto vi = *m_property_iterator++;
        if (!index_value{vi}.valid()) {
            throw out_of_range_exception{vi};
        }

        if (ki >= m_layer->key_table_size()) {
            throw out_of_range_exception{ki};
        }

        if (vi >= m_layer->value_table_size()) {
            throw out_of_range_exception{vi};
        }

        return {ki, vi};
    }

    template <typename TFunc>
    bool feature::for_each_property(TFunc&& func) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer->version() < 3);

        for (auto it = m_properties.begin(); it != m_properties.end();) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            vtzero_assert(it != m_properties.end());
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi};
            }

            if (!std::forward<TFunc>(func)(property{m_layer->key(ki), m_layer->value(vi)})) {
                return false;
            }
        }

        return true;
    }

    template <typename TFunc>
    bool feature::for_each_property_indexes(TFunc&& func) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer->version() < 3);

        for (auto it = m_properties.begin(); it != m_properties.end();) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            vtzero_assert(it != m_properties.end());
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi};
            }

            if (!std::forward<TFunc>(func)(index_value_pair{ki, vi})) {
                return false;
            }
        }

        return true;
    }

    namespace detail {

        template <typename THandler, typename TIterator>
        bool decode_property(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end);

        template <typename TIterator>
        void skip_complex_value(std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw format_exception{"Properties list is missing value"};
            }

            const uint64_t complex_value = *it++;

            const auto vt = complex_value & 0x0fu;
            if (vt > static_cast<std::size_t>(detail::complex_value_type::max)) {
                throw format_exception{"unknown complex value type: " + std::to_string(vt)};
            }

            const auto cvt = static_cast<detail::complex_value_type>(vt);

            if (cvt == detail::complex_value_type::cvt_list) {
                auto vp = complex_value >> 4u;
                while (vp > 0) {
                    --vp;
                    skip_complex_value(depth + 1, it, end);
                }
            } else if (cvt == detail::complex_value_type::cvt_map) {
                auto vp = complex_value >> 4u;
                while (vp > 0) {
                    --vp;
                    ++it; // skip key
                    if (it == end) {
                        throw format_exception{"Properties map is missing value"};
                    }
                    skip_complex_value(depth + 1, it, end);
                }
            }
        }

        template <typename THandler, typename TIterator>
        bool decode_complex_value(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw format_exception{"Properties list is missing value"};
            }

            const uint64_t complex_value = *it++;

            const auto vt = complex_value & 0x0fu;
            if (vt > static_cast<std::size_t>(detail::complex_value_type::max)) {
                throw format_exception{"unknown complex value type: " + std::to_string(vt)};
            }

            auto vp = complex_value >> 4u;
            switch (static_cast<detail::complex_value_type>(vt)) {
                case detail::complex_value_type::cvt_inline_sint:
                    if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), protozero::decode_zigzag64(vp), depth)) {
                        return false;
                    }
                    break;
                case detail::complex_value_type::cvt_inline_uint:
                    if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), vp, depth)) {
                        return false;
                    }
                    break;
                case detail::complex_value_type::cvt_bool:
                    switch (vp) {
                        case 0:
                            if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), false, depth)) {
                                return false;
                            }
                            break;
                        case 1:
                            if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), true, depth)) {
                                return false;
                            }
                            break;
                        case 2:
                            if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), null_type{}, depth)) {
                                return false;
                            }
                            break;
                        default:
                            throw format_exception{"invalid value for bool/null value: " + std::to_string(vp)};
                    }
                    break;
                case detail::complex_value_type::cvt_double:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!detail::call_double_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), layer.double_table().at(idx), depth)) {
                            return false;
                        }
                    }
                    break;
                case detail::complex_value_type::cvt_float:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!detail::call_float_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), layer.float_table().at(idx), depth)) {
                            return false;
                        }
                    }
                    break;
                case detail::complex_value_type::cvt_string:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!detail::call_string_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), layer.string_table().at(vp), depth)) {
                            return false;
                        }
                    }
                    break;
                case detail::complex_value_type::cvt_sint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!detail::call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), protozero::decode_zigzag64(layer.int_table().at(idx)), depth)) {
                            return false;
                        }
                    }
                    break;
                case detail::complex_value_type::cvt_uint:
                    {
                        // if the value of vp is so large that the static_cast
                        // will change it, the data is invalid anyway and we
                        // don't care which index it points to
                        const index_value idx{static_cast<uint32_t>(vp)};
                        if (!detail::call_int_value_index<THandler>(std::forward<THandler>(handler), idx, depth)) {
                            return false;
                        }
                        if (!detail::call_attribute_value<THandler>(std::forward<THandler>(handler), layer.int_table().at(idx), depth)) {
                            return false;
                        }
                    }
                    break;
                case detail::complex_value_type::cvt_list:
                    if (!detail::call_start_list_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_complex_value(handler, layer, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!detail::call_end_list_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
                case detail::complex_value_type::cvt_map:
                    if (!detail::call_start_map_attribute<THandler>(std::forward<THandler>(handler), static_cast<std::size_t>(vp), depth)) {
                        return false;
                    }
                    while (vp > 0) {
                        --vp;
                        if (!decode_property(handler, layer, depth + 1, it, end)) {
                            return false;
                        }
                    }
                    if (!detail::call_end_map_attribute<THandler>(std::forward<THandler>(handler), depth)) {
                        return false;
                    }
                    break;
            }

            return true;
        }

        template <typename THandler, typename TIterator>
        bool decode_property(THandler&& handler, const layer& layer, std::size_t depth, TIterator& it, TIterator end) {
            const auto ki = static_cast<uint32_t>(*it++);
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            if (!detail::call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, depth)) {
                skip_complex_value(depth, it, end);
                return true;
            }

            if (!detail::call_attribute_key<THandler>(std::forward<THandler>(handler), layer.key(ki), depth)) {
                skip_complex_value(depth, it, end);
                return true;
            }

            return decode_complex_value(handler, layer, depth, it, end);
        }

    } // namespace detail

    template <typename THandler>
    detail::get_result_t<THandler> feature::decode_attributes(THandler&& handler) const {
        vtzero_assert(valid());
        vtzero_assert(m_layer != nullptr);

        // vt2 properties
        for (auto it = m_properties.begin(); it != m_properties.end();) {
            const uint32_t ki = *it++;
            if (!index_value{ki}.valid()) {
                throw out_of_range_exception{ki};
            }

            vtzero_assert(it != m_properties.end());
            const uint32_t vi = *it++;
            if (!index_value{vi}.valid()) {
                throw out_of_range_exception{vi};
            }

            if (!detail::call_key_index<THandler>(std::forward<THandler>(handler), index_value{ki}, static_cast<std::size_t>(0))) {
                continue;
            }
            if (!detail::call_attribute_key<THandler>(std::forward<THandler>(handler), m_layer->key(ki), static_cast<std::size_t>(0))) {
                continue;
            }
            if (!detail::call_value_index<THandler>(std::forward<THandler>(handler), index_value{vi}, static_cast<std::size_t>(0))) {
                break;
            }

            bool keep_going = false;
            switch (m_layer->value(vi).type()) {
                case property_value_type::string_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).string_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::float_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).float_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::double_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).double_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::int_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).int_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::uint_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).uint_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::sint_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).sint_value(), static_cast<std::size_t>(0));
                    break;
                case property_value_type::bool_value:
                    keep_going = detail::call_attribute_value<THandler>(std::forward<THandler>(handler), m_layer->value(vi).bool_value(), static_cast<std::size_t>(0));
                    break;
            }
            if (!keep_going) {
                break;
            }
        }

        // vt3 attributes
        for (auto it = m_attributes.begin(); it != m_attributes.end();) {
            if (!detail::decode_property(handler, *m_layer, 0, it, m_attributes.end())) {
                break;
            }
        }

        return detail::get_result<THandler>::of(std::forward<THandler>(handler));
    }

} // namespace vtzero

#endif // VTZERO_LAYER_HPP
