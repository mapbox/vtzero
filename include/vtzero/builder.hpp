#ifndef VTZERO_BUILDER_HPP
#define VTZERO_BUILDER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file builder.hpp
 *
 * @brief Contains the classes and functions to build vector tiles.
 */

#include "builder_impl.hpp"
#include "feature_builder_impl.hpp"
#include "geometry.hpp"
#include "scaling.hpp"
#include "types.hpp"
#include "vector_tile.hpp"

#include <protozero/pbf_builder.hpp>

#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace vtzero {

    /**
     * Used to build vector tiles. Whenever you are building a new vector
     * tile, start with an object of this class and add layers. After all
     * the data is added, call serialize().
     *
     * @code
     * layer some_existing_layer = ...;
     *
     * tile_builder builder;
     * layer_builder layer_roads{builder, "roads"};
     * builder.add_existing_layer(some_existing_layer);
     * ...
     * std::string data = builder.serialize();
     * @endcode
     */
    class tile_builder {

        friend class layer_builder;

        std::vector<std::unique_ptr<detail::layer_builder_base>> m_layers;

        /**
         * Add a new layer to the vector tile based on an existing layer. The
         * new layer will have the same name, version, and extent as the
         * existing layer. The new layer will not contain any features. This
         * method is handy when copying some (but not all) data from an
         * existing layer.
         */
        detail::layer_builder_impl* add_layer(const layer& layer) {
            const auto ptr = new detail::layer_builder_impl{layer.name(), layer.version(), layer.extent()};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        /**
         * Add a new layer to the vector tile with the specified name, version,
         * and extent.
         *
         * @tparam TString Some string type (const char*, std::string,
         *         vtzero::data_view) or something that converts to one of
         *         these types.
         * @param name Name of this layer.
         * @param version Version of this layer (only version 1 and 2 are
         *                supported)
         * @param extent Extent used for this layer.
         */
        template <typename TString>
        detail::layer_builder_impl* add_layer(TString&& name, uint32_t version, uint32_t extent) {
            const auto ptr = new detail::layer_builder_impl{std::forward<TString>(name), version, extent};
            m_layers.emplace_back(ptr);
            return ptr;
        }

    public:

        /// Constructor
        tile_builder() = default;

        /// Destructor
        ~tile_builder() noexcept = default;

        /// Tile builders can not be copied.
        tile_builder(const tile_builder&) = delete;

        /// Tile builders can not be copied.
        tile_builder& operator=(const tile_builder&) = delete;

        /// Tile builders can be moved.
        tile_builder(tile_builder&&) = default;

        /// Tile builders can be moved.
        tile_builder& operator=(tile_builder&&) = default;

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param data Reference to some data that must be a valid encoded
         *        layer.
         */
        void add_existing_layer(data_view&& data) {
            m_layers.emplace_back(new detail::layer_builder_existing{std::forward<data_view>(data)});
        }

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param layer Reference to the layer to be copied.
         */
        void add_existing_layer(const layer& layer) {
            add_existing_layer(layer.data());
        }

        /**
         * Serialize the data accumulated in this builder into a vector tile.
         * The data will be appended to the specified buffer. The buffer
         * doesn't have to be empty.
         *
         * @param buffer Buffer to append the encoded vector tile to.
         */
        void serialize(std::string& buffer) const {
            std::size_t estimated_size = 0;
            for (const auto& layer : m_layers) {
                estimated_size += layer->estimated_size();
            }

            buffer.reserve(buffer.size() + estimated_size);

            protozero::pbf_builder<detail::pbf_tile> pbf_tile_builder{buffer};
            for (const auto& layer : m_layers) {
                layer->build(pbf_tile_builder);
            }
        }

        /**
         * Serialize the data accumulated in this builder into a vector_tile
         * and return it.
         *
         * If you want to use an existing buffer instead, use the serialize()
         * method taking a std::string& as parameter.
         *
         * @returns std::string Buffer with encoded vector_tile data.
         */
        std::string serialize() const {
            std::string data;
            serialize(data);
            return data;
        }

    }; // class tile_builder

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

        /**
         * Add a feature from an existing layer to the new layer. The feature
         * will be copied completely over to the new layer including its
         * geometry and all its properties.
         */
        void add_feature(const feature& feature);

    }; // class layer_builder

    namespace detail {

        template <int Dimensions>
        class elevations_policy {

        public:

            void add(int64_t /*value*/) const noexcept {
            }

            void serialize(protozero::pbf_builder<detail::pbf_feature>& /*builder*/) const noexcept {
            }

        }; // class elevations_policy

        template <>
        class elevations_policy<3> {

            std::vector<int64_t> m_values;

        public:

            void add(int64_t value) {
                m_values.push_back(value);
            }

            void serialize(protozero::pbf_builder<detail::pbf_feature>& builder) const {
                builder.add_packed_sint64(detail::pbf_feature::elevations, m_values.cbegin(), m_values.cend());
            }

        }; // class elevations_policy<3>

        template <bool WithGeometricAttributes>
        class geometric_attributes_policy {

        public:

        }; // class geometric_attributes_policy

        template <>
        class geometric_attributes_policy<true> {

        public:

        }; // class geometric_attributes_policy<true>

    } // namespace detail

    /**
     * Parent class for the point_feature_builder, linestring_feature_builder
     * and polygon_feature_builder classes. You can not instantiate this class
     * directly, use it through its derived classes.
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class feature_builder : public detail::feature_builder_base {

        static_assert(Dimensions == 2 || Dimensions == 3, "Need 2 or 3 dimensions");

        class countdown_value {

            uint32_t m_value = 0;

        public:

            countdown_value() noexcept = default;

            ~countdown_value() noexcept {
                assert_is_zero();
            }

            countdown_value(const countdown_value&) = delete;

            countdown_value& operator=(const countdown_value&) = delete;

            countdown_value(countdown_value&& other) noexcept :
                m_value(other.m_value) {
                other.m_value = 0;
            }

            countdown_value& operator=(countdown_value&& other) noexcept {
                m_value = other.m_value;
                other.m_value = 0;
                return *this;
            }

            uint32_t value() const noexcept {
                return m_value;
            }

            void set(const uint32_t value) noexcept {
                m_value = value;
            }

            void decrement() {
                vtzero_assert(m_value > 0 && "too many calls to set_point()");
                --m_value;
            }

            void assert_is_zero() const noexcept {
                vtzero_assert_in_noexcept_function(m_value == 0 &&
                                                   "not enough calls to set_point()");
            }

        }; // countdown_value

    protected:

        /// The elevations store (if using 3D geometries).
        detail::elevations_policy<Dimensions> m_elevations;

        /// The geometric attributes store (optional).
        detail::geometric_attributes_policy<WithGeometricAttributes> m_geometric_attributes;

        /// Encoded geometry.
        protozero::packed_field_uint32 m_pbf_geometry{};

        /// Number of points still to be set for the geometry to be complete.
        countdown_value m_num_points;

        /// Last point (used to calculate delta between coordinates)
        point m_cursor{0, 0};

        /// Constructor.
        explicit feature_builder(detail::layer_builder_impl* layer) :
            feature_builder_base(layer) {
        }

        /// Helper function to check size isn't too large
        template <typename T>
        uint32_t check_num_points(T size) {
            if (size >= (1ul << 29u)) {
                throw geometry_exception{"Maximum of 2^29 - 1 points allowed in geometry"};
            }
            return static_cast<uint32_t>(size);
        }

        /// Helper function to make sure we have everything before adding a property
        void prepare_to_add_property_vt2() {
            if (m_pbf_geometry.valid()) {
                m_num_points.assert_is_zero();
                m_pbf_geometry.commit();
            }
            if (!m_pbf_tags.valid()) {
                m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
            }
        }

        /// Helper function to make sure we have everything before adding a property
        void prepare_to_add_property_vt3() {
            if (m_pbf_geometry.valid()) {
                m_num_points.assert_is_zero();
                m_pbf_geometry.commit();
            }
            if (!m_pbf_attributes.valid()) {
                m_pbf_attributes = {m_feature_writer, detail::pbf_feature::attributes};
            }
        }

    public:

        /**
         * If the feature was not committed, the destructor will roll back all
         * the changes.
         */
        ~feature_builder() {
            try {
                rollback();
            } catch (...) {
                // ignore exceptions
            }
        }

        /// Builder classes can not be copied
        feature_builder(const feature_builder&) = delete;

        /// Builder classes can not be copied
        feature_builder& operator=(const feature_builder&) = delete;

        /// Builder classes can be moved
        feature_builder(feature_builder&& other) noexcept = default;

        /// Builder classes can be moved
        feature_builder& operator=(feature_builder&& other) = default;

        /**
         * Set the ID of this feature.
         *
         * You can only call this method once and it must be before calling
         * any method manipulating the geometry.
         *
         * @param id The ID.
         */
        void set_id(uint64_t id) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          !valid_properties() &&
                          "Call set_id() before setting the geometry or adding properties");
            set_integer_id_impl(id);
        }

        /**
         * Set the ID of this feature.
         *
         * You can only call this method once and it must be before calling
         * any method manipulating the geometry.
         *
         * @param id The ID.
         */
        void set_string_id(data_view id) {
            vtzero_assert(version() == 3 && "string_id is only allowed in version 3");
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          !m_pbf_tags.valid() &&
                          "Call set_id() before setting the geometry or adding properties");
            set_string_id_impl(id);
        }

        /**
         * Copy the ID of an existing feature to this feature. If the
         * feature doesn't have an ID, no ID is set.
         *
         * You can only call this method once and it must be before calling
         * any method manipulating the geometry.
         *
         * @param feature The feature to copy the ID from.
         */
        void copy_id(const feature& feature) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call copy_id() after commit() or rollback()");
            vtzero_assert(!m_pbf_geometry.valid() &&
                          !valid_properties() &&
                          "Call copy_id() before setting the geometry or adding properties");
            copy_id_impl(feature);
        }

        /**
         * Add a property to this feature. Can only be called after all the
         * methods manipulating the geometry.
         *
         * @tparam TProp Can be type index_value_pair or property.
         * @param prop The property to add.
         *
         * @pre layer version < 3
         */
        template <typename TProp>
        void add_property(TProp&& prop) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            vtzero_assert(version() < 3);
            prepare_to_add_property_vt2();
            add_property_impl_vt2(std::forward<TProp>(prop));
        }

        /**
         * Copy all properties of an existing feature to the one being built.
         *
         * @param feature The feature to copy the properties from.
         *
         * @pre layer version < 3
         */
        void copy_properties(const feature& feature) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call copy_properties() after commit() or rollback()");
            vtzero_assert(version() < 3);
            prepare_to_add_property_vt2();
            feature.for_each_property([this](const property& prop) {
                add_property_impl_vt2(prop);
                return true;
            });
        }

        /**
         * Copy all properties of an existing feature to the one being built
         * using a property_mapper.
         *
         * @tparam TMapper Must be the property_mapper class or something
         *                 equivalent.
         * @param feature The feature to copy the properties from.
         * @param mapper Instance of the property_mapper class.
         *
         * @pre layer version < 3
         */
        template <typename TMapper>
        void copy_properties(const feature& feature, TMapper& mapper) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call copy_properties() after commit() or rollback()");
            vtzero_assert(version() < 3);
            prepare_to_add_property_vt2();
            feature.for_each_property_indexes([this, &mapper](const index_value_pair& idxs) {
                add_property_impl_vt2(mapper(idxs));
                return true;
            });
        }

        /**
         * Add a property to this feature. Can only be called after all the
         * methods manipulating the geometry.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TValue Can be type index_value or property_value or
         *         encoded_property or anything that converts to it.
         * @param key The key.
         * @param value The value.
         *
         * @pre layer version < 3
         */
        template <typename TKey, typename TValue>
        void add_property(TKey&& key, TValue&& value) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            vtzero_assert(version() < 3);
            prepare_to_add_property_vt2();
            add_property_impl_vt2(std::forward<TKey>(key), std::forward<TValue>(value));
        }

        /**
         * Add an attribute key to this feature. Can only be called after all
         * the methods manipulating the geometry.
         *
         * This function works with layers of version 1 to 3. After this, you
         * have to call attribute_value() or another function adding
         * an attribute value.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @param key The key.
         * @param depth Optional depth (will be ignored).
         */
        template <typename TKey>
        void attribute_key(TKey&& key, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            if (version() < 3) {
                prepare_to_add_property_vt2();
            } else {
                prepare_to_add_property_vt3();
            }
            add_key_internal(std::forward<TKey>(key));
        }

        /**
         * Add the value of a scalar attribute to this feature. Can only be
         * called after add_attribute_key() was called or after the
         * start_list_attribute() function.
         *
         * This function works with layers of version 1 to 3. Not all value
         * types are allowed in all versions.
         *
         * @tparam TValue Can be of any scalar attribute type.
         * @param value The value.
         * @param depth Optional depth (will be ignored).
         */
        template <typename TValue>
        void attribute_value(TValue&& value, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            if (version() < 3) {
                add_value_internal_vt2(std::forward<TValue>(value));
            } else {
                add_value_internal_vt3(std::forward<TValue>(value));
            }
        }

        /**
         * Add a scalar attribute to this feature. Can only be called after all
         * the methods manipulating the geometry.
         *
         * This function works with layers of version 1 to 3. Not all value
         * types are allowed in all versions.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TValue Can be of any scalar attribute type.
         * @param key The key.
         * @param value The value.
         */
        template <typename TKey, typename TValue>
        void add_scalar_attribute(TKey&& key, TValue&& value) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            if (version() < 3) {
                prepare_to_add_property_vt2();
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt2(std::forward<TValue>(value));
            } else {
                prepare_to_add_property_vt3();
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt3(std::forward<TValue>(value));
            }
        }

        /**
         * Start to add a list attribute to this feature. Can only be called
         * after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attribute values using
         * attribute_value().
         *
         * @param size The number of elements in the list.
         * @param depth Optional depth (will be ignored).
         *
         * @pre layer version is 3
         */
        void start_list_attribute(std::size_t size, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            add_complex_value(detail::complex_value_type::cvt_list, size);
        }

        /**
         * Start to add a number list attribute to this feature. Can only be
         * called after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attribute values using
         * number_list_value().
         *
         * @param size The number of elements in the list.
         * @param index The index of the attribute scalings used.
         * @param depth Optional depth (will be ignored).
         *
         * @pre layer version is 3
         */
        void start_number_list(std::size_t size, index_value index, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            add_complex_value(detail::complex_value_type::cvt_number_list, size);
            add_direct_value(index.value());
        }

        /**
         * Start to add a number list attribute to this feature. Can only be called
         * after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attribute values using
         * number_list_value().
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @param key The key.
         * @param index The index of the attribute scalings used.
         * @param size The number of elements in the list.
         *
         * @pre layer version is 3
         */
        template <typename TKey>
        void start_number_list_with_key(TKey&& key, std::size_t size, index_value index) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            prepare_to_add_property_vt3();
            add_key_internal(std::forward<TKey>(key));
            add_complex_value(detail::complex_value_type::cvt_number_list, size);
            add_direct_value(index.value());
        }

        /**
         * Add a value to a number list.
         *
         * @param value The value.
         *
         * @pre layer version is 3
         */
        void number_list_value(uint64_t value) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            add_direct_value(value);
        }

        /**
         * Start to add a list attribute to this feature. Can only be called
         * after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attribute values using
         * attribute_value().
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @param key The key.
         * @param size The number of elements in the list.
         *
         * @pre layer version is 3
         */
        template <typename TKey>
        void start_list_attribute_with_key(TKey&& key, std::size_t size) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            prepare_to_add_property_vt3();
            add_key_internal(std::forward<TKey>(key));
            add_complex_value(detail::complex_value_type::cvt_list, size);
        }

        /**
         * Start to add a map attribute to this feature. Can only be called
         * after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attributes using the other
         * attribute functions.
         *
         * @param size The number of elements in the map.
         * @param depth Optional depth (will be ignored).
         *
         * @pre layer version is 3
         */
        void start_map_attribute(std::size_t size, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "map attributes are only allowed in version 3 layers");
            add_complex_value(detail::complex_value_type::cvt_map, size);
        }

        /**
         * Start to add a map attribute to this feature. Can only be called
         * after all the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * Call this function, then add size attributes using the other
         * attribute functions.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @param key The key.
         * @param size The number of elements in the map.
         *
         * @pre layer version is 3
         */
        template <typename TKey>
        void start_map_attribute_with_key(TKey&& key, std::size_t size) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call attribute functions after commit() or rollback()");
            vtzero_assert(version() == 3 && "map attributes are only allowed in version 3 layers");
            prepare_to_add_property_vt3();
            add_key_internal(std::forward<TKey>(key));
            add_complex_value(detail::complex_value_type::cvt_map, size);
        }

        /**
         * Add a list attribute to this feature. Can only be called after all
         * the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TIterator Iterator yielding some value types.
         * @param key The key.
         * @param begin Iterator to start of a collection.
         * @param end Iterator one past the end of a collection.
         * @param size The number of elements in the list.
         *
         * @pre layer version is 3
         * @pre the iterators must yield exactly size values
         */
        template <typename TKey, typename TIterator>
        void add_list_attribute(TKey&& key, TIterator begin, TIterator end, std::size_t size) {
            start_list_attribute_with_key(std::forward<TKey>(key), size);
            while (begin != end) {
                attribute_value(*begin++);
            }
        }

        /**
         * Add a list attribute to this feature. Can only be called after all
         * the methods manipulating the geometry.
         *
         * This function works with layers of version 3 only.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TIterator Iterator yielding some value types.
         * @param key The key.
         * @param begin Iterator to start of a collection.
         * @param end Iterator one past the end of a collection.
         *
         * @pre layer version is 3
         * @pre internally std::distance(begin, end) is used, so the TIterator
         *      must allow multiple passes
         */
        template <typename TKey, typename TIterator>
        void add_list_attribute(TKey&& key, TIterator begin, TIterator end) {
            const auto size = static_cast<std::size_t>(std::distance(begin, end));
            add_list_attribute(std::forward<TKey>(key), begin, end, size);
        }

        /**
         * Copy all attributes of an existing feature to the one being built.
         *
         * @param feature The feature to copy the attributes from.
         */
        void copy_attributes(const feature& feature) {
            vtzero_assert(m_feature_writer.valid() &&
                          "Can not call copy_attributes() after commit() or rollback()");
            if (version() < 3) {
                prepare_to_add_property_vt2();
                feature.for_each_property([this](const property& prop) {
                    add_property_impl_vt2(prop);
                    return true;
                });
            } else {
                feature.decode_attributes(*this);
            }
        }

        /**
         * Commit this feature. Call this after all the details of this
         * feature have been added. If this is not called, the feature
         * will be rolled back when the destructor of the feature_builder is
         * called.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void commit() {
            if (m_feature_writer.valid()) {
                vtzero_assert((m_pbf_geometry.valid() || valid_properties()) &&
                              "Can not call commit before geometry was added");
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                    m_elevations.serialize(m_feature_writer);
                }
                do_commit();
            }
        }

        /**
         * Rollback this feature. Removed all traces of this feature from
         * the layer_builder. Useful when you started creating a feature
         * but then find out that its geometry is invalid or something like
         * it. This will also happen automatically when the feature_builder
         * is destructed and commit() hasn't been called on it.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void rollback() {
            if (m_feature_writer.valid()) {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.rollback();
                }
                do_rollback();
            }
        }

    }; // class feature_builder

    /**
     * Used for adding a feature with a point geometry to a layer. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)point geometry using add_point(), add_points() and
     *   set_point(), or add_points_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::point_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_point(10, 20) // add point geometry
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class point_feature_builder : public feature_builder<Dimensions, WithGeometricAttributes> {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit point_feature_builder(layer_builder layer) :
            feature_builder<Dimensions, WithGeometricAttributes>(&layer.get_layer_impl()) {
            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POINT));
        }

        /**
         * Add a single unscaled_point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const unscaled_point p) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->m_pbf_geometry.valid() &&
                          !this->valid_properties() &&
                          "add_point() can only be called once");
            this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
            this->m_pbf_geometry.add_element(detail::command_move_to(1));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
            this->m_elevations.add(p.z);
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const point p) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->m_pbf_geometry.valid() &&
                          !this->valid_properties() &&
                          "add_point() can only be called once");
            this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
            this->m_pbf_geometry.add_element(detail::command_move_to(1));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param x X coordinate of the point to add.
         * @param y Y coordinate of the point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const int32_t x, const int32_t y) {
            add_point(point{x, y});
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void add_point(TPoint&& p) {
            add_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Declare the intent to add a multipoint geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the multipoint geometry.
         *
         * @pre @code count > 0 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_points(uint32_t count) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->m_pbf_geometry.valid() &&
                          "can not call add_points() twice or mix with add_point()");
            vtzero_assert(!this->valid_properties() &&
                          "add_points() has to be called before properties are added");
            vtzero_assert(count > 0 && count < (1ul << 29u) && "add_points() must be called with 0 < count < 2^29");
            this->m_num_points.set(count);
            this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
            this->m_pbf_geometry.add_element(detail::command_move_to(count));
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @param p The point.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(this->m_pbf_geometry.valid() &&
                          "call add_points() before set_point()");
            vtzero_assert(!this->valid_properties() &&
                          "set_point() has to be called before properties are added");
            this->m_num_points.decrement();
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
            this->m_cursor = p;
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the multipoint geometry.
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Add the points from the specified container as multipoint geometry
         * to this feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_points_from_container(const TContainer& container) {
            add_points(this->check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class point_feature_builder

    /**
     * Used for adding a feature with a (multi)linestring geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)linestring geometry using add_linestring() or
     *   add_linestring_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::linestring_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_linestring(2);
     * fb.set_point(10, 10);
     * fb.set_point(10, 20);
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class linestring_feature_builder : public feature_builder<Dimensions, WithGeometricAttributes> {

        bool m_start_line = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit linestring_feature_builder(layer_builder layer) :
            feature_builder<Dimensions, WithGeometricAttributes>(&layer.get_layer_impl()) {
            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::LINESTRING));
        }

        /**
         * Declare the intent to add a linestring geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the linestring.
         *
         * @pre @code count > 1 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_linestring(const uint32_t count) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->valid_properties() &&
                          "add_linestring() has to be called before properties are added");
            vtzero_assert(count > 1 && count < (1ul << 29u) && "add_linestring() must be called with 1 < count < 2^29");
            this->m_num_points.assert_is_zero();
            if (!this->m_pbf_geometry.valid()) {
                this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
            }
            this->m_num_points.set(count);
            m_start_line = true;
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @param p The point.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(this->m_pbf_geometry.valid() &&
                          "call add_linestring() before set_point()");
            vtzero_assert(!this->valid_properties() &&
                          "set_point() has to be called before properties are added");
            this->m_num_points.decrement();
            if (m_start_line) {
                this->m_pbf_geometry.add_element(detail::command_move_to(1));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
                this->m_pbf_geometry.add_element(detail::command_line_to(this->m_num_points.value()));
                m_start_line = false;
            } else {
                if (p == this->m_cursor) {
                    throw geometry_exception{"Zero-length segments in linestrings are not allowed."};
                }
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
            }
            this->m_cursor = p;
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the multilinestring geometry opened with
         * add_linestring().
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_linestring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Add the points from the specified container as a linestring geometry
         * to this feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container or if two consecutive points in the container
         *         are identical.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_linestring_from_container(const TContainer& container) {
            add_linestring(this->check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class linestring_feature_builder

    /**
     * Used for adding a feature with a (multi)polygon geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the (multi)polygon geometry using add_ring() or
     *   add_ring_from_container().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::polygon_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.add_ring(5);
     * fb.set_point(10, 10);
     * ...
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class polygon_feature_builder : public feature_builder<Dimensions, WithGeometricAttributes> {

        point m_first_point{0, 0};
        bool m_start_ring = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit polygon_feature_builder(layer_builder layer) :
            feature_builder<Dimensions, WithGeometricAttributes>(&layer.get_layer_impl()) {
            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POLYGON));
        }

        /**
         * Declare the intent to add a ring with *count* points to this
         * feature.
         *
         * @param count The number of points in the ring.
         *
         * @pre @code count > 3 && count < 2^29 @endcode
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void add_ring(const uint32_t count) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->valid_properties() &&
                          "add_ring() has to be called before properties are added");
            vtzero_assert(count > 3 && count < (1ul << 29u) && "add_ring() must be called with 3 < count < 2^29");
            this->m_num_points.assert_is_zero();
            if (!this->m_pbf_geometry.valid()) {
                this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
            }
            this->m_num_points.set(count);
            m_start_ring = true;
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @param p The point.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(this->m_pbf_geometry.valid() &&
                          "call add_ring() before set_point()");
            vtzero_assert(!this->valid_properties() &&
                          "set_point() has to be called before properties are added");
            this->m_num_points.decrement();
            if (m_start_ring) {
                m_first_point = p;
                this->m_pbf_geometry.add_element(detail::command_move_to(1));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
                this->m_pbf_geometry.add_element(detail::command_line_to(this->m_num_points.value() - 1));
                m_start_ring = false;
                this->m_cursor = p;
            } else if (this->m_num_points.value() == 0) {
                if (p != m_first_point) {
                    throw geometry_exception{"Last point in a ring must be the same as the first point."};
                }
                // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                this->m_pbf_geometry.add_element(detail::command_close_path());
            } else {
                if (p == this->m_cursor) {
                    throw geometry_exception{"Zero-length segments in linestrings are not allowed."};
                }
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
                this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
                this->m_cursor = p;
            }
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the ring opened with add_ring().
         *
         * @tparam TPoint A type that can be converted to vtzero::point using
         *         the create_vtzero_point function.
         * @param p The point to add.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *         This exception is also thrown when the last point in the
         *         ring is not equal to the first point, because this would
         *         not create a closed ring.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TPoint>
        void set_point(TPoint&& p) {
            set_point(create_vtzero_point(std::forward<TPoint>(p)));
        }

        /**
         * Close a ring opened with add_ring(). This can be called for the
         * last point (which will be equal to the first point) in the ring
         * instead of calling set_point().
         *
         * @pre There must have been *count* - 1 calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        void close_ring() {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(this->m_pbf_geometry.valid() &&
                          "Call add_ring() before you can call close_ring()");
            vtzero_assert(!this->valid_properties() &&
                          "close_ring() has to be called before properties are added");
            vtzero_assert(this->m_num_points.value() == 1 &&
                          "wrong number of points in ring");
            this->m_pbf_geometry.add_element(detail::command_close_path());
            this->m_num_points.decrement();
        }

        /**
         * Add the points from the specified container as a ring to this
         * feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         *
         * @throws geometry_exception If there are more than 2^32-1 members in
         *         the container or if two consecutive points in the container
         *         are identical or if the last point is not the same as the
         *         first point.
         *
         * @pre You must not have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_ring_from_container(const TContainer& container) {
            add_ring(this->check_num_points(container.size()));
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class polygon_feature_builder

    /**
     * Used for adding a feature to a layer using an existing geometry. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_id().
     * * Add the geometry using set_geometry().
     * * Optionally add any number of properties using add_property().
     *
     * @code
     * auto geom = ... // get geometry from a feature you are reading
     * ...
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::geometry_feature_builder fb{lb};
     * fb.set_id(123); // optionally set ID
     * fb.set_geometry(geom) // add geometry
     * fb.add_property("foo", "bar"); // add property
     * @endcode
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class geometry_feature_builder : public detail::feature_builder_base {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit geometry_feature_builder(layer_builder layer) :
            feature_builder_base(&layer.get_layer_impl()) {
        }

        /**
         * If the feature was not committed, the destructor will roll back all
         * the changes.
         */
        ~geometry_feature_builder() noexcept {
            try {
                rollback();
            } catch (...) {
                // ignore exceptions
            }
        }

        /// Feature builders can not be copied.
        geometry_feature_builder(const geometry_feature_builder&) = delete;

        /// Feature builders can not be copied.
        geometry_feature_builder& operator=(const geometry_feature_builder&) = delete;

        /// Feature builders can be moved.
        geometry_feature_builder(geometry_feature_builder&&) noexcept = default;

        /// Feature builders can be moved.
        geometry_feature_builder& operator=(geometry_feature_builder&&) noexcept = default;

        /**
         * Set the ID of this feature.
         *
         * You can only call this function once and it must be before calling
         * set_geometry().
         *
         * @param id The ID.
         */
        void set_id(uint64_t id) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!this->valid_properties());
            set_integer_id_impl(id);
        }

        /**
         * Set the ID of this feature.
         *
         * You can only call this method once and it must be before calling
         * set_geometry().
         *
         * @param id The ID.
         */
        void set_string_id(data_view id) {
            vtzero_assert(version() == 3 && "string_id is only allowed in version 3");
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call set_id() after commit() or rollback()");
            vtzero_assert(!this->valid_properties());
            set_string_id_impl(id);
        }

        /**
         * Copy the ID of an existing feature to this feature. If the
         * feature doesn't have an ID, no ID is set.
         *
         * You can only call this function once and it must be before calling
         * set_geometry().
         *
         * @param feature The feature to copy the ID from.
         */
        void copy_id(const feature& feature) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call copy_id() after commit() or rollback()");
            vtzero_assert(!this->valid_properties());
            copy_id_impl(feature);
        }

        /**
         * Set the geometry of this feature.
         *
         * You can only call this method once and it must be before calling the
         * add_property() method.
         *
         * @param geometry The geometry.
         */
        void set_geometry(const geometry& geometry) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not add geometry after commit() or rollback()");
            vtzero_assert(!this->valid_properties());
            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(geometry.type()));
            this->m_feature_writer.add_string(detail::pbf_feature::geometry, geometry.data());
            m_pbf_tags = {this->m_feature_writer, detail::pbf_feature::tags};
        }

        /**
         * Add a property to this feature. Can only be called after the
         * set_geometry method.
         *
         * @tparam TProp Can be type index_value_pair or property.
         * @param prop The property to add.
         *
         * @pre layer version < 3
         */
        template <typename TProp>
        void add_property(TProp&& prop) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            vtzero_assert(version() < 3);
            add_property_impl_vt2(std::forward<TProp>(prop));
        }

        /**
         * Add a property to this feature. Can only be called after the
         * set_geometry method.
         *
         * @tparam TKey Can be type index_value or data_view or anything that
         *         converts to it.
         * @tparam TValue Can be type index_value or property_value or
         *         encoded_property or anything that converts to it.
         * @param key The key.
         * @param value The value.
         *
         * @pre layer version < 3
         */
        template <typename TKey, typename TValue>
        void add_property(TKey&& key, TValue&& value) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call add_property() after commit() or rollback()");
            vtzero_assert(version() < 3);
            add_property_impl_vt2(std::forward<TKey>(key), std::forward<TValue>(value));
        }

        /**
         * Copy all properties of an existing feature to the one being built.
         *
         * @param feature The feature to copy the properties from.
         *
         * @pre layer version < 3
         */
        void copy_properties(const feature& feature) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call copy_properties() after commit() or rollback()");
            vtzero_assert(version() < 3);
            feature.for_each_property([this](const property& prop) {
                add_property_impl_vt2(prop);
                return true;
            });
        }

        /**
         * Copy all properties of an existing feature to the one being built
         * using a property_mapper.
         *
         * @tparam TMapper Must be the property_mapper class or something
         *                 equivalent.
         * @param feature The feature to copy the properties from.
         * @param mapper Instance of the property_mapper class.
         *
         * @pre layer version < 3
         */
        template <typename TMapper>
        void copy_properties(const feature& feature, TMapper& mapper) {
            vtzero_assert(this->m_feature_writer.valid() &&
                          "Can not call copy_properties() after commit() or rollback()");
            vtzero_assert(version() < 3);
            feature.for_each_property_indexes([this, &mapper](const index_value_pair& idxs) {
                add_property_impl_vt2(mapper(idxs));
                return true;
            });
        }

        /**
         * Commit this feature. Call this after all the details of this
         * feature have been added. If this is not called, the feature
         * will be rolled back when the destructor of the feature_builder is
         * called.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void commit() {
            if (this->m_feature_writer.valid()) {
                vtzero_assert(this->valid_properties() &&
                              "Can not call commit() before geometry was added");
                do_commit();
            }
        }

        /**
         * Rollback this feature. Removed all traces of this feature from
         * the layer_builder. Useful when you started creating a feature
         * but then find out that its geometry is invalid or something like
         * it. This will also happen automatically when the feature_builder
         * is destructed and commit() hasn't been called on it.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void rollback() {
            if (this->m_feature_writer.valid()) {
                do_rollback();
            }
        }

    }; // class geometry_feature_builder

    /// alias for 2D point feature builder
    using point_2d_feature_builder = point_feature_builder<2, false>;

    /// alias for 2D linestring feature builder
    using linestring_2d_feature_builder = linestring_feature_builder<2, false>;

    /// alias for 2D polygon feature builder
    using polygon_2d_feature_builder = polygon_feature_builder<2, false>;

    /// alias for 2D geometry feature builder
    using geometry_2d_feature_builder = geometry_feature_builder<2, false>;

    inline void layer_builder::add_feature(const feature& feature) {
        vtzero_assert(m_layer->version() < 3);
        geometry_2d_feature_builder feature_builder{*this};
        feature_builder.copy_id(feature);
        feature_builder.set_geometry(feature.geometry());
        feature.for_each_property([&feature_builder](const property& p) {
            feature_builder.add_property(p);
            return true;
        });
        feature_builder.commit();
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP
