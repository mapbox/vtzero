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

#include "detail/builder_impl.hpp"
#include "detail/feature_builder_impl.hpp"
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

        class countdown_value {

            uint32_t m_value = 0;

        public:

            countdown_value() noexcept = default;

            ~countdown_value() noexcept {
                vtzero_assert_in_noexcept_function(is_zero());
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

            void decrement() noexcept {
                --m_value;
            }

        }; // class countdown_value

    } // namespace detail


    /**
     * @brief Builder class for writing features
     *
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
    template <int Dimensions>
    class feature_builder : public detail::feature_builder_base {

        static_assert(Dimensions == 2 || Dimensions == 3, "Dimensions template parameter can only be 2 or 3");

        int64_t m_value = 0;

    protected:

        /// Encoded geometry.
        protozero::packed_field_uint32 m_pbf_geometry{};

        /// Number of points still to be set for the geometry to be complete.
        detail::countdown_value m_num_points;

        /// Previous point (used to calculate delta between coordinates).
        point<Dimensions> m_cursor{};

        /// The number of knots still to write.
        detail::countdown_value m_num_knots;

        /**
         * Enter the "want_geometry" stage. Do nothing if we are already in
         * the "want_geometry" stage.
         *
         * @pre Feature builder is in stage "want_id" or "has_id".
         * @post Feature builder is in stage "has_geometry".
         */
        void enter_stage_geometry(GeomType type) {
            if (m_stage == detail::stage::want_id || m_stage == detail::stage::has_id) {
                m_stage = detail::stage::want_geometry;
                this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(type));
                this->m_pbf_geometry = {this->m_feature_writer, detail::pbf_feature::geometry};
                return;
            }
            vtzero_assert(m_stage == detail::stage::want_geometry);
        }

        /**
         * Commit a started geometry.
         *
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "has_geometry".
         */
        void commit_geometry() {
            if (!m_pbf_geometry.valid()) {
                return;
            }
            vtzero_assert(m_num_points.is_zero());
            vtzero_assert(m_num_knots.is_zero());
            m_pbf_geometry.commit();
            if (Dimensions == 3 && !elevations().empty()) {
                m_feature_writer.add_packed_sint32(detail::pbf_feature::elevations, elevations().cbegin(), elevations().cend());
                elevations().clear();
            }
            if (!knots().empty()) {
                m_feature_writer.add_packed_uint64(detail::pbf_feature::spline_knots, knots().cbegin(), knots().cend());
                knots().clear();
            }
            m_stage = detail::stage::has_geometry;
        }

        /**
         * Enter the "want_attrs" stage. Do nothing if we are already in the
         * "want_attrs" or "want_geom_attrs" stage.
         */
        void enter_stage_attributes() {
            vtzero_assert(version() == 3);
            if (m_stage == detail::stage::want_geometry) {
                commit_geometry();
                m_pbf_attributes = {m_feature_writer, detail::pbf_feature::attributes};
                m_stage = detail::stage::want_attrs;
                return;
            }
            if (m_stage == detail::stage::has_geometry) {
                m_pbf_attributes = {m_feature_writer, detail::pbf_feature::attributes};
                m_stage = detail::stage::want_attrs;
                return;
            }
            vtzero_assert(m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs);
        }

        /**
         * Enter the "want_tags" stage. Do nothing if we are already in the
         * "want_tags" stage.
         *
         * @pre Feature builder is in stage "want_geometry", "has_geometry",
         *      or "want_tags".
         * @post Feature builder is in stage "want_tags".
         */
        void enter_stage_tags() {
            if (m_stage == detail::stage::want_geometry) {
                commit_geometry();
                m_pbf_attributes = {m_feature_writer, detail::pbf_feature::tags};
                m_stage = detail::stage::want_tags;
                return;
            }
            if (m_stage == detail::stage::has_geometry) {
                m_pbf_attributes = {m_feature_writer, detail::pbf_feature::tags};
                m_stage = detail::stage::want_tags;
                return;
            }
            vtzero_assert(m_stage == detail::stage::want_tags &&
                          "Must be in stage 'want_geometry', 'has_geometry', or 'want_tags'");
        }

        /// Add specified point to the data doing the zigzag encoding.
        void add_point_impl(const point<Dimensions> p) {
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            this->m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
            if (Dimensions == 3) {
                elevations().push_back(p.get_z());
            }
        }

        /// Add specified point to the data with delta encoding.
        void set_point_impl(const point<Dimensions> p) {
            point<Dimensions> q{sub(p.x, m_cursor.x), sub(p.y, m_cursor.y)};
            if (Dimensions == 3) {
                q.set_z(sub(p.get_z(), m_cursor.get_z()));
            }
            add_point_impl(q);
            m_cursor = p;
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
         *
         * @pre Feature builder is in stage "want_geometry", "has_geometry",
         *      or "want_attrs".
         * @post Feature builder is in stage "want_geom_attrs".
         */
        void switch_to_geometric_attributes() {
            if (m_stage == detail::stage::want_geometry) {
                commit_geometry();
            } else {
                vtzero_assert(m_stage == detail::stage::has_geometry || m_stage == detail::stage::want_attrs);
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.commit();
                }
            }
            m_stage = detail::stage::want_geom_attrs;
            m_pbf_attributes = {m_feature_writer, detail::pbf_feature::geometric_attributes};
        }

        /**
         * Copy the geometry from the geometry of an existing feature.
         *
         * If the geometry is a 3D geometry, elevations data will be copied.
         * If the geometry is a spline, control points and knots will be
         * copied. Geometric attributes are not copied.
         *
         * @param feature The feature to copy the geometry from.
         *
         * @pre Feature builder is in stage "want_id" or "has_id".
         * @post Feature builder is in stage "has_geometry".
         */
        void copy_geometry(const feature& feature) {
            vtzero_assert((m_stage == detail::stage::want_id || m_stage == detail::stage::has_id) &&
                          "copy_geometry() can only be called in stage 'want_id' or 'has_id'.");

            this->m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(feature.geometry_type()));
            this->m_feature_writer.add_string(detail::pbf_feature::geometry, feature.geometry_data());
            if (feature.has_3d_geometry()) {
                this->m_feature_writer.add_string(detail::pbf_feature::elevations, feature.elevations_data());
            }
            if (!feature.knots_data().empty()) {
                this->m_feature_writer.add_uint32(detail::pbf_feature::spline_degree, feature.spline_degree());
                this->m_feature_writer.add_string(detail::pbf_feature::spline_knots, feature.knots_data());
            }

            m_stage = detail::stage::has_geometry;
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
            vtzero_assert(version() < 3 &&
                          "Calling add_property() is not allowed in version 3 layers.");

            this->enter_stage_tags();
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
            vtzero_assert(version() < 3 &&
                          "Calling add_property() is not allowed in version 3 layers.");

            this->enter_stage_tags();
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
            if (version() < 3) {
                this->enter_stage_tags();
            } else {
                this->enter_stage_attributes();
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
            if (version() < 3) {
                vtzero_assert(m_stage == detail::stage::want_tags &&
                              "Must call attribute_key() before attribute_value().");
                add_value_internal_vt2(std::forward<TValue>(value));
            } else {
                vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                              "Must call attribute_key() before attribute_value().");
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
            if (version() < 3) {
                this->enter_stage_tags();
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt2(std::forward<TValue>(value));
            } else {
                this->enter_stage_attributes();
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
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");
            vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                          "Must call attribute_key() before start_list_attribute().");

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
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");
            vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                          "Must call attribute_key() before start_number_list().");

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
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");

            this->enter_stage_attributes();
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
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");
            vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                          "Must call start_number_list(with_key)() before number_list_value().");

            add_direct_value(protozero::encode_zigzag64(value - m_value) + 1);
            m_value = value;
        }

        /**
         * Add a null value to a number list.
         *
         * @pre layer version is 3
         */
        void number_list_null_value(std::size_t /*depth*/ = 0) {
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");
            vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                          "Must call start_number_list(with_key)() before number_list_value_null().");

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
            vtzero_assert(version() == 3 &&
                          "list attributes are only allowed in version 3 layers");

            this->enter_stage_attributes();
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
            vtzero_assert(version() == 3 &&
                          "map attributes are only allowed in version 3 layers");
            vtzero_assert((m_stage == detail::stage::want_attrs || m_stage == detail::stage::want_geom_attrs) &&
                          "Must call attribute_key() before start_map_attribute().");

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
            vtzero_assert(version() == 3 &&
                          "map attributes are only allowed in version 3 layers");

            this->enter_stage_attributes();
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
            if (version() < 3) {
                this->enter_stage_tags();
            } else {
                this->enter_stage_attributes();
            }
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
         *
         * @pre Complete geometry on this feature must have been set.
         */
        void commit() {
            if (!m_feature_writer.valid()) {
                return;
            }
            vtzero_assert(m_stage != detail::stage::want_id && m_stage != detail::stage::has_id);
            commit_geometry();
            do_commit();
        }

        /**
         * Roll back this feature. Removes all traces of this feature from
         * the layer_builder. Useful when you started creating a feature
         * but then find out that its geometry is invalid or something like
         * it. This will also happen automatically when the feature_builder
         * is destructed and commit() hasn't been called on it.
         *
         * Once a feature has been committed or rolled back, further calls
         * to commit() or rollback() don't do anything.
         */
        void rollback() {
            if (!m_feature_writer.valid()) {
                return;
            }
            if (m_pbf_geometry.valid()) {
                m_pbf_geometry.rollback();
            }
            if (Dimensions == 3) {
                elevations().clear();
            }
            knots().clear();
            do_rollback();
        }

    }; // class feature_builder

    /**
     * @brief Builder class for point features
     *
     * Used for adding a feature with a point geometry to a layer. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using set_integer_id()/set_string_id().
     * * Add the (multi)point geometry using add_point() or add_points() and
     *   set_point().
     * * Optionally add any number of attributes.
     *
     * Single point:
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::point_feature_builder<> fb{lb};
     * fb.set_integer_id(123);
     * fb.add_point(10, 20);
     * fb.add_scalar_attribute("foo", "bar");
     * fb.commit();
     * @endcode
     *
     * Multipoint:
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::point_feature_builder<> fb{lb};
     * fb.set_integer_id(123);
     * fb.add_points(2);
     * fb.set_point(vtzero::point_2d{10, 20});
     * fb.set_point(vtzero::point_2d{20, 20});
     * fb.add_scalar_attribute("foo", "bar");
     * fb.commit();
     * @endcode
     */
    template <int Dimensions = 2>
    class point_feature_builder : public feature_builder<Dimensions> {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         *
         * @post Feature builder is in stage "want_id".
         */
        explicit point_feature_builder(layer_builder layer) :
            feature_builder<Dimensions>(layer) {
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre Feature builder is in stage "want_id" or "has_id".
         * @post Feature builder is in stage "has_geometry".
         */
        void add_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::want_id || this->m_stage == detail::stage::has_id);

            this->enter_stage_geometry(GeomType::POINT);
            this->m_pbf_geometry.add_element(detail::command_move_to(1));
            this->add_point_impl(p);
            this->commit_geometry();
        }

        /**
         * Declare the intent to add a multipoint geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the multipoint geometry.
         *
         * @pre @code count > 0 && count < 2^29 @endcode
         *
         * @pre Feature builder is in stage "want_id, "has_id", or
         *      "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void add_points(uint32_t count) {
            vtzero_assert(this->m_stage == detail::stage::want_id || this->m_stage == detail::stage::has_id);
            vtzero_assert(count > 0 && count < (1ul << 29u) &&
                          "add_points() must be called with 0 < count < 2^29");

            this->enter_stage_geometry(GeomType::POINT);
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
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
            vtzero_assert(!this->m_num_points.is_zero());

            this->m_num_points.decrement();
            this->set_point_impl(p);
        }

    }; // class point_feature_builder

    /**
     * @brief Builder class for linestring features
     *
     * Used for adding a feature with a (multi)linestring geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_integer_id()/set_string_id().
     * * Add the (multi)linestring geometry using add_linestring() and
     *   set_point().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::linestring_feature_builder<> fb{lb};
     * fb.set_integer_id(123);
     * fb.add_linestring(2);
     * fb.set_point(vtzero::point_2d{10, 20});
     * fb.set_point(vtzero::point_2d{20, 20});
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * fb.commit();
     * @endcode
     */
    template <int Dimensions = 2>
    class linestring_feature_builder : public feature_builder<Dimensions> {

        bool m_start_line = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         *
         * @post Feature builder is in stage "want_id".
         */
        explicit linestring_feature_builder(layer_builder layer) :
            feature_builder<Dimensions>(layer) {
        }

        /**
         * Declare the intent to add a linestring geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the linestring.
         *
         * @pre @code count > 1 && count < 2^29 @endcode
         *
         * @pre Feature builder is in stage "want_id", "has_id", or
         *      "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void add_linestring(const uint32_t count) {
            vtzero_assert(count > 1 && count < (1ul << 29u) &&
                          "add_linestring() must be called with 1 < count < 2^29");
            vtzero_assert(this->m_num_points.is_zero());

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
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
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

    }; // class linestring_feature_builder

    /**
     * @brief Builder class for polygon features
     *
     * Used for adding a feature with a (multi)polygon geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_integer_id()/set_string_id().
     * * Add the (multi)polygon geometry using add_ring(), set_point(), and
     *   close_ring().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::polygon_feature_builder<> fb{lb};
     * fb.set_integer_id(123);
     * fb.add_ring(5);
     * fb.set_point(vtzero::point_2d{10, 10});
     * ...
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * fb.commit();
     * @endcode
     */
    template <int Dimensions = 2>
    class polygon_feature_builder : public feature_builder<Dimensions> {

        point<Dimensions> m_first_point{};
        bool m_start_ring = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         *
         * @post Feature builder is in stage "want_id".
         */
        explicit polygon_feature_builder(layer_builder layer) :
            feature_builder<Dimensions>(layer) {
        }

        /**
         * Declare the intent to add a ring with *count* points to this
         * feature.
         *
         * @param count The number of points in the ring.
         *
         * @pre @code count > 3 && count < 2^29 @endcode
         * @pre Feature builder is in stage "want_id", "has_id", or
         *      "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void add_ring(const uint32_t count) {
            vtzero_assert(count > 3 && count < (1ul << 29u) &&
                          "add_ring() must be called with 3 < count < 2^29");
            vtzero_assert(this->m_num_points.is_zero());

            this->enter_stage_geometry(GeomType::POLYGON);
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
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
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
         * Close a ring opened with add_ring(). This can be called for the
         * last point (which will be equal to the first point) in the ring
         * instead of calling set_point().
         *
         * @pre There must have been *count* - 1 calls to set_point()
         *      already after a call to add_ring(count).
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void close_ring() {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
            vtzero_assert(this->m_num_points.value() == 1 &&
                          "wrong number of points in ring");

            this->m_pbf_geometry.add_element(detail::command_close_path());
            this->m_num_points.decrement();
        }

    }; // class polygon_feature_builder

    /**
     * @brief Builder class for spline features
     *
     * Used for adding a feature with a (multi)spline geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using set_integer_id()/set_string_id().
     * * Add the (multi)spline geometry using add_spline(), set_point(),
     *   and set_knot().
     * * Optionally add any number of attributes.
     *
     * @code
     * vtzero::tile_builder tb;
     * vtzero::layer_builder lb{tb};
     * vtzero::spline_feature_builder<> fb{lb, 2}; // degree == 2
     * fb.set_integer_id(123);
     * fb.add_spline(2, 0); // 2 control points
     * fb.set_point(vtzero::point_2d{10, 20}); // 1.
     * fb.set_point(vtzero::point_2d{20, 20}); // 2.
     * fb.set_knot(1); // 2 control points + 2 (degree) + 1 == number of knots
     * ...
     * fb.set_knot(5);
     * fb.add_scalar_attribute("foo", "bar"); // add attribute
     * fb.commit();
     * @endcode
     */
    template <int Dimensions = 2>
    class spline_feature_builder : public feature_builder<Dimensions> {

        int64_t m_knots_old_value = 0;
        uint32_t m_spline_degree;
        bool m_start_line = false;

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         * @param spline_degree The degree of the spline.
         *
         * @pre @code spline_degree == 2 || spline_degree == 3 @endcode
         * @post Feature builder is in stage "want_id".
         */
        explicit spline_feature_builder(layer_builder layer, const uint32_t spline_degree = 2) :
            feature_builder<Dimensions>(layer),
            m_spline_degree(spline_degree) {
            vtzero_assert(layer.version() == 3);
            vtzero_assert(spline_degree == 2 || spline_degree == 3);

            this->m_feature_writer.add_uint32(detail::pbf_feature::spline_degree, spline_degree);
        }

        /**
         * Declare the intent to add a spline geometry with *count* control
         * points to this feature.
         *
         * @param count The number of control points in the spline.
         * @param knots_index The index of the scaling used for the knots.
         *
         * @pre @code count > 1 && count < 2^29 @endcode
         * @pre All splines started with add_spline() before must be complete.
         * @pre Feature builder is in stage "want_id", "has_id", or "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void add_spline(const uint32_t count, const index_value knots_index) {
            vtzero_assert(count > 1 && count < (1ul << 29u) &&
                          "add_spline() must be called with 1 < count < 2^29");
            vtzero_assert(this->m_num_points.is_zero() &&
                          "Previously started spline is not complete (missing control points).");
            vtzero_assert(this->m_num_knots.is_zero() &&
                          "Previously started spline is not complete (missing knots).");

            this->enter_stage_geometry(GeomType::SPLINE);
            this->m_num_points.set(count);
            this->m_num_knots.set(count + m_spline_degree + 1);
            this->knots().reserve(this->knots().size() + this->m_num_knots.value());
            this->knots().push_back(create_complex_value(detail::complex_value_type::cvt_number_list, this->m_num_knots.value()));
            this->knots().push_back(knots_index.value());
            m_start_line = true;
            m_knots_old_value = 0;
        }

        /**
         * Set a control point in the (multi)spline geometry opened with
         * add_spline().
         *
         * @param p The point.
         *
         * @throws geometry_exception if the point set is the same as the
         *         previous point. This would create zero-length segments
         *         which are not allowed according to the vector tile spec.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_spline(count, index).
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void set_point(const point<Dimensions> p) {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
            vtzero_assert(!this->m_num_points.is_zero());

            this->m_num_points.decrement();
            if (m_start_line) {
                this->m_pbf_geometry.add_element(detail::command_move_to(1));
                this->set_point_impl(p);
                this->m_pbf_geometry.add_element(detail::command_line_to(this->m_num_points.value()));
                m_start_line = false;
            } else {
                if (p == this->m_cursor) {
                    throw geometry_exception{"Zero-length segments in splines are not allowed."};
                }
                this->set_point_impl(p);
            }
        }

        /**
         * Set a knot in the (multi)spline geometry opened with add_spline().
         *
         * @param value The value of the knot. Must be scaled with the
         *        scaling used for this geometry.
         *
         * @pre There must have been less than *count + degree + 1* calls to
         *      set_knot() already after a call to add_spline(count, index).
         *
         * @pre Feature builder is in stage "want_geometry".
         * @post Feature builder is in stage "want_geometry".
         */
        void set_knot(const int64_t value) {
            vtzero_assert(this->m_stage == detail::stage::want_geometry);
            vtzero_assert(!this->m_num_knots.is_zero());

            this->m_num_knots.decrement();
            const auto delta = value - m_knots_old_value;
            this->knots().push_back(protozero::encode_zigzag64(delta) + 1);
            m_knots_old_value = value;
        }

    }; // class spline_feature_builder

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP
