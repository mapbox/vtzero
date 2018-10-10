#ifndef VTZERO_LAYER_BUILDER_HPP
#define VTZERO_LAYER_BUILDER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file layer_builder.hpp
 *
 * @brief Contains the layer_builder class.
 */

#include "builder_impl.hpp"
#include "scaling.hpp"
#include "tile_builder.hpp"
#include "types.hpp"

#include <type_traits>
#include <utility>

namespace vtzero {

    /**
     * The layer_builder is used to add a new layer to a vector tile that is
     * being built.
     */
    class layer_builder {

        vtzero::detail::layer_builder_impl* m_layer;

        template <int Dimensions, bool WithGeometricAttributes>
        friend class point_feature_builder;

        template <int Dimensions, bool WithGeometricAttributes>
        friend class linestring_feature_builder;

        template <int Dimensions, bool WithGeometricAttributes>
        friend class polygon_feature_builder;

        template <int Dimensions, bool WithGeometricAttributes>
        friend class geometry_feature_builder;

        vtzero::detail::layer_builder_impl& get_layer_impl() noexcept {
            return *m_layer;
        }

        template <typename T>
        using is_layer = std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, layer>;

    public:

        /**
         * Construct a layer_builder to build a new layer with the same name,
         * version, and extent as an existing layer.
         *
         * @param tile The tile builder we want to create this layer in.
         * @param layer Existing layer we want to use the name, version, and
         *        extent from
         */
        layer_builder(vtzero::tile_builder& tile, const layer& layer) :
            m_layer(tile.add_layer(layer)) {
        }

        /**
         * Construct a layer_builder to build a completely new layer.
         *
         * @tparam TString Some string type (such as std::string or const char*)
         * @param tile The tile builder we want to create this layer in.
         * @param name The name of the new layer.
         * @param version The vector tile spec version of the new layer.
         * @param extent The extent of the new layer.
         */
        template <typename TString, typename std::enable_if<!is_layer<TString>::value, int>::type = 0>
        layer_builder(vtzero::tile_builder& tile, TString&& name, uint32_t version = 2, uint32_t extent = 4096) :
            m_layer(tile.add_layer(std::forward<TString>(name), version, extent)) {
        }

        /// Get the elevation scaling currently set.
        const scaling& elevation_scaling() const noexcept {
            return m_layer->elevation_scaling();
        }

        /**
         * Get the attribute scaling with the specified index.
         *
         * @param index The index of the scaling requested.
         * @returns scaling
         * @throws std::out_of_range if the index is out of range.
         */
        const scaling& attribute_scaling(index_value index) const {
            return m_layer->attribute_scaling(index);
        }

        /**
         * Set the scaling for elevations. If this is not set, the default
         * scaling is assumed.
         *
         * @param s The scaling.
         * @pre Layer being built must be a version 3 layer.
         */
        void set_elevation_scaling(const scaling& s) {
            m_layer->set_elevation_scaling(s);
        }

        /**
         * Add a scaling for geometric attributes.
         *
         * @param s The scaling.
         * @returns Index of this scaling.
         * @pre Layer being built must be a version 3 layer.
         */
        index_value add_attribute_scaling(const scaling& s) {
            return m_layer->add_attribute_scaling(s);
        }

        /**
         * Add key to the keys table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param text The key.
         * @returns The index value of this key.
         */
        index_value add_key_without_dup_check(const data_view text) {
            return m_layer->add_key_without_dup_check(text);
        }

        /**
         * Add key to the keys table. This function will consult the internal
         * index in the layer to make sure the key is only in the table once.
         * It will either return the index value of an existing key or add the
         * new key and return its index value.
         *
         * @param text The key.
         * @returns The index value of this key.
         */
        index_value add_key(const data_view text) {
            return m_layer->add_key(text);
        }

        /**
         * Add value to the values table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(const property_value value) {
            return m_layer->add_value_without_dup_check(value);
        }

        /**
         * Add value to the values table without checking for duplicates. This
         * function is usually used when an external index is used which takes
         * care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(const encoded_property_value& value) {
            return m_layer->add_value_without_dup_check(value);
        }

        /**
         * Add value to the values table. This function will consult the
         * internal index in the layer to make sure the value is only in the
         * table once. It will either return the index value of an existing
         * value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(const property_value value) {
            return m_layer->add_value(value);
        }

        /**
         * Add value to the values table. This function will consult the
         * internal index in the layer to make sure the value is only in the
         * table once. It will either return the index value of an existing
         * value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(const encoded_property_value& value) {
            return m_layer->add_value(value);
        }

        /**
         * Add string value to the string_values table without checking for
         * duplicates. This function is usually used when an external index is
         * used which takes care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_string_value_without_dup_check(const data_view value) {
            return m_layer->add_string_value_without_dup_check(value);
        }

        /**
         * Add string value to the string_values table. This function will
         * consult the internal index in the layer to make sure the value is
         * only in the table once. It will either return the index value of an
         * existing value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_string_value(const data_view value) {
            return m_layer->add_string_value(value);
        }

        /**
         * Add double value to the double_values table without checking for
         * duplicates. This function is usually used when an external index is
         * used which takes care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(double value) {
            return m_layer->add_double_value_without_dup_check(value);
        }

        /**
         * Add double value to the double_values table. This function will
         * consult the internal index in the layer to make sure the value is
         * only in the table once. It will either return the index value of an
         * existing value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(double value) {
            return m_layer->add_double_value(value);
        }

        /**
         * Add float value to the float_values table without checking for
         * duplicates. This function is usually used when an external index is
         * used which takes care of the duplication check.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value_without_dup_check(float value) {
            return m_layer->add_float_value_without_dup_check(value);
        }

        /**
         * Add float value to the float_values table. This function will
         * consult the internal index in the layer to make sure the value is
         * only in the table once. It will either return the index value of an
         * existing value or add the new value and return its index value.
         *
         * @param value The property value.
         * @returns The index value of this value.
         */
        index_value add_value(float value) {
            return m_layer->add_float_value(value);
        }

    }; // class layer_builder

} // namespace vtzero

#endif // VTZERO_LAYER_BUILDER_HPP
