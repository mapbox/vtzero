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
#include "layer_builder.hpp"
#include "tile_builder.hpp"
#include "types.hpp"
#include "vector_tile.hpp"

#include <protozero/pbf_builder.hpp>

#include <cstdlib>
#include <utility>
#include <vector>

namespace vtzero {

    namespace detail {

        template <int Dimensions>
        class elevations_policy {

        public:

            void add(int32_t /*value*/) const noexcept {
            }

            void serialize(protozero::pbf_builder<detail::pbf_feature>& /*builder*/) const noexcept {
            }

        }; // class elevations_policy

        template <>
        class elevations_policy<3> {

            std::vector<int32_t> m_values;

        public:

            void add(int32_t value) {
                m_values.push_back(value);
            }

            void serialize(protozero::pbf_builder<detail::pbf_feature>& builder) const {
                builder.add_packed_sint32(detail::pbf_feature::elevations, m_values.cbegin(), m_values.cend());
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
     * and polygon_feature_builder classes. Usually you do not instantiate this
     * class directly, but use it through its derived classes.
     *
     * There is one case where you instantiate this class directly: When you
     * want to copy a complete geometry from some other feature. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_integer/string_id().
     * * Add the geometry using copy_geometry().
     * * Optionally add any number of attributes.
     *
     * @code
     * auto geom = ... // get geometry from a feature you are reading
     * ...
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::geometry_feature_builder fb{lb};
     * fb.set_integer_id(123); // optionally set ID
     * fb.copy_geometry(geom) // add geometry
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * @endcode
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class feature_builder : public detail::feature_builder_base {

        static_assert(Dimensions == 2 || Dimensions == 3, "Need 2 or 3 dimensions");

        int64_t m_value = 0;

    protected:

        /// The elevations store (if using 3D geometries).
        detail::elevations_policy<Dimensions> m_elevations;

        /// The geometric attributes store (optional).
        detail::geometric_attributes_policy<WithGeometricAttributes> m_geometric_attributes;

        /// Encoded geometry.
        protozero::packed_field_uint32 m_pbf_geometry{};

        /**
         * Enter the "geometry" stage. Do nothing if we are already in the
         * "geometry" stage.
         */
        void enter_stage_geometry(GeomType type) {
            if (m_stage == detail::stage::id || m_stage == detail::stage::has_id) {
                m_stage = detail::stage::geometry;
                this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(type));
                this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
                return;
            }
            vtzero_assert(m_stage == detail::stage::geometry);
        }

        /**
         * Enter the "attributes" stage. Do nothing if we are already in the
         * "attributes" stage.
         */
        void enter_stage_attributes() {
            if (m_stage == detail::stage::geometry) {
                m_pbf_geometry.commit();
                m_stage = detail::stage::attributes;
                return;
            }
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
        }

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit feature_builder(layer_builder layer) :
            feature_builder_base(&layer.get_layer_impl()) {
        }

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
        feature_builder& operator=(feature_builder&& other) noexcept = default;

        /**
         * Call this after you have written all normal attributes, before
         * you are writing the first geometric attribute.
         */
        void switch_to_geometric_attributes() {
            if (m_stage == detail::stage::geometry) {
                m_pbf_geometry.commit();
            } else {
                vtzero_assert(m_stage == detail::stage::attributes);
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.commit();
                }
            }
            m_stage = detail::stage::geom_attributes;
            m_pbf_attributes = {m_feature_writer, detail::pbf_feature::geometric_attributes};
        }

        /**
         * Copy the geometry from the geometry of an existing feature.
         *
         * @param feature The feature to copy the geometry from.
         */
        void copy_geometry(const feature& feature) {
            vtzero_assert(m_stage == detail::stage::id || m_stage == detail::stage::has_id);
            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(feature.geometry_type()));
            this->m_feature_writer.add_string(detail::pbf_feature::geometry, feature.geometry_data());
            if (feature.has_3d_geometry()) {
                this->m_feature_writer.add_string(detail::pbf_feature::elevations, feature.elevations_data());
            }
            m_stage = detail::stage::attributes;
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
            this->enter_stage_attributes();
            vtzero_assert(version() < 3);
            prepare_to_add_property_vt2();
            add_property_impl_vt2(std::forward<TProp>(prop));
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
            this->enter_stage_attributes();
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
            this->enter_stage_attributes();
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
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
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
            this->enter_stage_attributes();
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
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
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
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
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
            this->enter_stage_attributes();
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            prepare_to_add_property_vt3();
            add_key_internal(std::forward<TKey>(key));
            add_complex_value(detail::complex_value_type::cvt_number_list, size);
            add_direct_value(index.value());
            m_value = 0;
        }

        /**
         * Add a value to a number list.
         *
         * @param value The value.
         *
         * @pre layer version is 3
         */
        void number_list_value(int64_t value, std::size_t /*depth*/ = 0) {
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            add_direct_value(protozero::encode_zigzag64(value - m_value) + 1);
            m_value = value;
        }

        /**
         * Add a null value to a number list.
         *
         * @pre layer version is 3
         */
        void number_list_null_value(std::size_t /*depth*/ = 0) {
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
            vtzero_assert(version() == 3 && "list attributes are only allowed in version 3 layers");
            add_direct_value(0);
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
            this->enter_stage_attributes();
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
            vtzero_assert(m_stage == detail::stage::attributes || m_stage == detail::stage::geom_attributes);
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
            this->enter_stage_attributes();
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
            this->enter_stage_attributes();
            feature.decode_attributes(*this);
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
     * Internal class which can not be instantiated. Instantiate one of its
     * derived classes instead.
     */
    template <int Dimensions, bool WithGeometricAttributes>
    class feature_builder_with_geometry : public feature_builder<Dimensions, WithGeometricAttributes> {

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

            bool is_zero() const noexcept {
                return m_value == 0;
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

        /// Number of points still to be set for the geometry to be complete.
        countdown_value m_num_points;

        /// Last point (used to calculate delta between coordinates)
        point<Dimensions> m_cursor{};

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit feature_builder_with_geometry(layer_builder layer) :
            feature_builder<Dimensions, WithGeometricAttributes>(layer) {
        }

        void set_point_impl(const point<Dimensions> p) {
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - this->m_cursor.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - this->m_cursor.y));
            if (Dimensions == 3) {
                this->m_elevations.add(p.get_z() - this->m_cursor.get_z());
            }
            this->m_cursor = p;
        }

    }; // class feature_builder_with_geometry

    /**
     * Used for adding a feature with a point geometry to a layer. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_integer/string_id().
     * * Add the (multi)point geometry using add_point(), add_points() and
     *   set_point(), or add_points_from_container().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::point_feature_builder fb{lb};
     * fb.set_integer_id(123); // optionally set ID
     * fb.add_point(10, 20) // add point geometry
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * @endcode
     */
    template <int Dimensions = 2, bool WithGeometricAttributes = false>
    class point_feature_builder : public feature_builder_with_geometry<Dimensions, WithGeometricAttributes> {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit point_feature_builder(layer_builder layer) :
            feature_builder_with_geometry<Dimensions, WithGeometricAttributes>(layer) {
        }

        /**
         * Add a single 3d point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre You must be in stage "id" or "has_id" to call this function.
         */
        void add_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::id || this->m_stage == detail::stage::has_id);
            this->enter_stage_geometry(GeomType::POINT);
            this->m_pbf_geometry.add_element(detail::command_move_to(1));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
            if (Dimensions == 3) {
                this->m_elevations.add(p.get_z());
            }
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param x X coordinate of the point to add.
         * @param y Y coordinate of the point to add.
         *
         * @pre You must be in stage "id" or "has_id" to call this function.
         */
        void add_point(const int32_t x, const int32_t y) {
            add_point(point_2d{x, y});
        }

        /**
         * Declare the intent to add a multipoint geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the multipoint geometry.
         *
         * @pre @code count > 0 && count < 2^29 @endcode
         *
         * @pre You must be in stage "id" or "has_id" to call this function.
         * @post You are in stage "geometry" after calling this function.
         */
        void add_points(uint32_t count) {
            vtzero_assert(this->m_stage == detail::stage::id || this->m_stage == detail::stage::has_id);
            this->enter_stage_geometry(GeomType::POINT);
            vtzero_assert(count > 0 && count < (1ul << 29u) && "add_points() must be called with 0 < count < 2^29");
            this->m_num_points.set(count);
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::geometry);
            vtzero_assert(!this->m_num_points.is_zero());
            this->m_num_points.decrement();
            this->set_point_impl(p);
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point_2d{x, y});
        }

    }; // class point_feature_builder

    /**
     * Used for adding a feature with a (multi)linestring geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_integer/string_id().
     * * Add the (multi)linestring geometry using add_linestring() or
     *   add_linestring_from_container().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::linestring_feature_builder fb{lb};
     * fb.set_integer_id(123); // optionally set ID
     * fb.add_linestring(2);
     * fb.set_point(10, 10);
     * fb.set_point(10, 20);
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * @endcode
     */
    template <int Dimensions = 2, bool WithGeometricAttributes = false>
    class linestring_feature_builder : public feature_builder_with_geometry<Dimensions, WithGeometricAttributes> {

        bool m_start_line = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit linestring_feature_builder(layer_builder layer) :
            feature_builder_with_geometry<Dimensions, WithGeometricAttributes>(layer) {
        }

        /**
         * Declare the intent to add a linestring geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the linestring.
         *
         * @pre @code count > 1 && count < 2^29 @endcode
         *
         * @pre You must be in stage "id", "has_id", or "geometry" to call
         *      this function.
         * @post You are in stage "geometry" after calling this function.
         */
        void add_linestring(const uint32_t count) {
            vtzero_assert(count > 1 && count < (1ul << 29u) && "add_linestring() must be called with 1 < count < 2^29");
            this->m_num_points.assert_is_zero();
            this->enter_stage_geometry(GeomType::LINESTRING);
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(!this->m_num_points.is_zero());
            this->m_num_points.decrement();
            if (m_start_line) {
                this->m_pbf_geometry.add_element(detail::command_move_to(1));
                this->set_point_impl(p);
                this->m_pbf_geometry.add_element(detail::command_line_to(this->m_num_points.value()));
                m_start_line = false;
            } else {
                if (p == this->m_cursor) {
                    throw geometry_exception{"Zero-length segments in linestrings are not allowed."};
                }
                this->set_point_impl(p);
            }
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point_2d{x, y});
        }

    }; // class linestring_feature_builder

    /**
     * Used for adding a feature with a (multi)polygon geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_integer/string_id().
     * * Add the (multi)polygon geometry using add_ring() or
     *   add_ring_from_container().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::polygon_feature_builder fb{lb};
     * fb.set_integer_id(123); // optionally set ID
     * fb.add_ring(5);
     * fb.set_point(10, 10);
     * ...
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * @endcode
     */
    template <int Dimensions = 2, bool WithGeometricAttributes = false>
    class polygon_feature_builder : public feature_builder_with_geometry<Dimensions, WithGeometricAttributes> {

        point<Dimensions> m_first_point{};
        bool m_start_ring = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit polygon_feature_builder(layer_builder layer) :
            feature_builder_with_geometry<Dimensions, WithGeometricAttributes>(layer) {
        }

        /**
         * Declare the intent to add a ring with *count* points to this
         * feature.
         *
         * @param count The number of points in the ring.
         *
         * @pre @code count > 3 && count < 2^29 @endcode
         *
         * @pre You must be in stage "id", "has_id", or "geometry" to call
         *      this function.
         */
        void add_ring(const uint32_t count) {
            vtzero_assert(count > 3 && count < (1ul << 29u) && "add_ring() must be called with 3 < count < 2^29");
            this->enter_stage_geometry(GeomType::POLYGON);
            this->m_num_points.assert_is_zero();
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(!this->m_num_points.is_zero());
            this->m_num_points.decrement();
            if (m_start_ring) {
                m_first_point = p;
                this->m_pbf_geometry.add_element(detail::command_move_to(1));
                this->set_point_impl(p);
                this->m_pbf_geometry.add_element(detail::command_line_to(this->m_num_points.value() - 1));
                m_start_ring = false;
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
                this->set_point_impl(p);
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
         * @pre You must be in stage "geometry" to call this function.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point_2d{x, y});
        }

        /**
         * Close a ring opened with add_ring(). This can be called for the
         * last point (which will be equal to the first point) in the ring
         * instead of calling set_point().
         *
         * @pre There must have been *count* - 1 calls to set_point()
         *      already after a call to add_ring(count).
         *
         * @pre You must be in stage "geometry" to call this function.
         */
        void close_ring() {
            vtzero_assert(this->m_stage == detail::stage::geometry);
            vtzero_assert(this->m_num_points.value() == 1 &&
                          "wrong number of points in ring");
            this->m_pbf_geometry.add_element(detail::command_close_path());
            this->m_num_points.decrement();
        }

    }; // class polygon_feature_builder

    /// alias for 2D point feature builder
    using point_2d_feature_builder = point_feature_builder<2, false>;

    /// alias for 2D linestring feature builder
    using linestring_2d_feature_builder = linestring_feature_builder<2, false>;

    /// alias for 2D polygon feature builder
    using polygon_2d_feature_builder = polygon_feature_builder<2, false>;

    /**
     * Copy a feature from an existing layer to a new layer. The feature
     * will be copied completely over to the new layer including its id,
     * geometry and all its attributes.
     */
    template <typename TLayerBuilder>
    void copy_feature(const feature& feature, TLayerBuilder& layer_builder) {
        feature_builder<3, true> feature_builder{layer_builder};
        feature_builder.copy_id(feature);
        feature_builder.copy_geometry(feature);
        feature_builder.copy_attributes(feature);
        feature_builder.commit();
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP
