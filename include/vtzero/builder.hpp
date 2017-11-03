#ifndef VTZERO_BUILDER_HPP
#define VTZERO_BUILDER_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

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
#include "types.hpp"
#include "vector_tile.hpp"

#include <protozero/pbf_builder.hpp>

#include <cstdlib>
#include <iostream>
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

        ~tile_builder() noexcept = default;

        tile_builder(const tile_builder&) = delete;
        tile_builder& operator=(const tile_builder&) = delete;

        tile_builder(tile_builder&&) = default;
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

        friend class geometry_feature_builder;
        friend class point_feature_builder;
        friend class linestring_feature_builder;
        friend class polygon_feature_builder;

        vtzero::detail::layer_builder_impl& get_layer_impl() noexcept {
            return *m_layer;
        };

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
        layer_builder(vtzero::tile_builder& tile, TString&& name, uint32_t version = 2, uint32_t extent = 4096):
            m_layer(tile.add_layer(std::forward<TString>(name), version, extent)) {
        }

        index_value add_key_without_dup_check(const data_view text) {
            return m_layer->add_key_without_dup_check(text);
        }

        index_value add_key(const data_view text) {
            return m_layer->add_key(text);
        }

        index_value add_value_without_dup_check(const data_view text) {
            return m_layer->add_value_without_dup_check(text);
        }

        index_value add_value(const data_view text) {
            return m_layer->add_value(text);
        }

        /**
         * Add a feature from an existing layer to the new layer. The feature
         * will be copied completely over to the new layer including its
         * geometry and all its properties.
         */
        void add_feature(const feature& feature);

    }; // class layer_builder

    /**
     * Used for adding a feature with a point geometry to a layer. After
     * creating an object of this class you can add data to the feature in a
     * specific order:
     *
     * * Optionally add the ID using setid().
     * * Add the (multi)point geometry using add_point(), add_points(), and
     *   set_point().
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
    class point_feature_builder : public detail::feature_builder {

    public:

        /**
         * Constructor
         *
         * @param layer The layer we want to create this feature in.
         */
        explicit point_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POINT));
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param p The point to add.
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const point p) {
            vtzero_assert(!m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            m_pbf_geometry.add_element(detail::command_move_to(1));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
            m_pbf_geometry.commit();
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @param x X coordinate of the point to add.
         * @param y Y coordinate of the point to add.
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        void add_point(const int32_t x, const int32_t y) {
            add_point(point{x, y});
        }

        /**
         * Add a single point as the geometry to this feature.
         *
         * @tparam T A type that can be converted to vtzero::point using the
         *         create_point function.
         * @param p The point to add.
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        template <typename T>
        void add_point(T p) {
            add_point(create_point(p));
        }

        /**
         * Declare the intent to add multipoint geometry with *count* points
         * to this feature.
         *
         * @param count The number of points in the multipolygon geometry.
         * @pre @code count > 0 @endcode
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        void add_points(uint32_t count) {
            vtzero_assert(!m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 0);
            m_num_points.set(count);
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
            m_pbf_geometry.add_element(detail::command_move_to(count));
        }

        /**
         * Set a point in the multipolygon geometry.
         *
         * @param p The point.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Call add_points() before set_point()");
            m_num_points.decrement();
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
            m_cursor = p;
        }

        /**
         * Set a point in the multipolygon geometry.
         *
         * @param x X coordinate of the point to set.
         * @param y Y coordinate of the point to set.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        /**
         * Set a point in the multipolygon geometry.
         *
         * @tparam T A type that can be converted to vtzero::point using the
         *         create_point function.
         * @param p The point to add.
         *
         * @pre There must have been less than *count* calls to set_point()
         *      already after a call to add_points(count).
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        template <typename T>
        void set_point(T p) {
            set_point(create_point(p));
        }

        /**
         * Add the points from an iterator range as multipolygon geometry
         * to this feature. This will determine the number of points in the
         * range using std::distance(), so it will not work on an input
         * iterator and might be more expensive than calling the overload
         * of this function taking a third parameter with the count.
         *
         * @tparam TIter Forward iterator type. Dereferencing must yield
         *         a vtzero::point or something convertible to it.
         * @param begin Iterator to the beginning of the range.
         * @param end Iterator one past the end of the range.
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        template <typename TIter>
        void add_points(TIter begin, TIter end) {
            add_points(std::distance(begin, end));
            for (; begin != end; ++begin) {
                set_point(*begin);
            }
            m_pbf_geometry.commit();
        }

        /**
         * Add the points from an iterator range as multipolygon geometry
         * to this feature. Use this function if you know the number of
         * points in the range.
         *
         * @tparam TIter Foward iterator type. Dereferencing must yield
         *         a vtzero::point or something convertible to it.
         * @param begin Iterator to the beginning of the range.
         * @param end Iterator one past the end of the range.
         * @param count The number of points in the range.
         *
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        template <typename TIter>
        void add_points(TIter begin, TIter end, uint32_t count) {
            add_points(count);
            for (; begin != end; ++begin) {
                set_point(*begin);
#ifndef NDEBUG
                --count;
#endif
            }
            vtzero_assert(count == 0 && "Iterators must yield exactly count points");
            m_pbf_geometry.commit();
        }

        /**
         * Add the points from the specified container as multipolygon geometry
         * to this feature.
         *
         * @tparam TContainer The container type. Must support the size()
         *         method, be iterable using a range for loop, and contain
         *         objects of type vtzero::point or something convertible to
         *         it.
         * @param container The container to read the points from.
         * @pre You must no have any calls to add_property() before calling
         *      this method.
         */
        template <typename TContainer>
        void add_points_from_container(const TContainer& container) {
            add_points(container.size());
            for (const auto& element : container) {
                set_point(element);
            }
            m_pbf_geometry.commit();
        }

    }; // class point_feature_builder

    /**
     * Used for adding a feature with a linestring geometry to a layer.
     * After creating an object of this class you can add data to the
     * feature in a specific order:
     *
     * * Optionally add the ID using setid().
     * * Add the (multi)linestring geometry using add_linestring() and
     *   set_point().
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
    class linestring_feature_builder : public detail::feature_builder {

        bool m_start_line = false;

    public:

        explicit linestring_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::LINESTRING));
        }

        void add_linestring(const uint32_t count) {
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 1);
            m_num_points.assert_is_zero();
            init_geometry();
            m_num_points.set(count);
            m_start_line = true;
        }

        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Add full geometry before adding tags");
            m_num_points.decrement();
            if (m_start_line) {
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points.value()));
                m_start_line = false;
            } else {
                vtzero_assert(p != m_cursor); // no zero-length segments
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
            }
            m_cursor = p;
        }

        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        template <typename T>
        void set_point(T p) {
            set_point(create_point(p));
        }

        template <typename TIter>
        void add_linestring(TIter begin, TIter end) {
            auto size = std::distance(begin, end);
            if (size > (1 << 29)) {
                throw format_exception{"a linestring can not contain more then 2^29 points"};
            }
            add_linestring(uint32_t(size));
            for (; begin != end; ++begin) {
                set_point(*begin);
            }
        }

        template <typename TIter>
        void add_linestring(TIter begin, TIter end, uint32_t count) {
            add_linestring(count);
            for (; begin != end; ++begin) {
                set_point(*begin);
#ifndef NDEBUG
                --count;
#endif
            }
            vtzero_assert(count == 0 && "Iterators must yield exactly count points");
        }

        template <typename TContainer>
        void add_linestring_from_container(const TContainer& container) {
            add_linestring(container.size());
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class linestring_feature_builder

    class polygon_feature_builder : public detail::feature_builder {

        point m_first_point{0, 0};
        bool m_start_ring = false;

    public:

        explicit polygon_feature_builder(layer_builder layer) :
            feature_builder(&layer.get_layer_impl()) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POLYGON));
        }

        void add_ring(const uint32_t count) {
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 3);
            m_num_points.assert_is_zero();
            init_geometry();
            m_num_points.set(count);
            m_start_ring = true;
        }

        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Call add_ring() before add_point()");
            m_num_points.decrement();
            if (m_start_ring) {
                m_first_point = p;
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points.value() - 1));
                m_start_ring = false;
                m_cursor = m_first_point;
            } else if (m_num_points.value() == 0) {
                vtzero_assert(m_first_point == p); // XXX
                // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                m_pbf_geometry.add_element(detail::command_close_path(1));
            } else {
                vtzero_assert(m_cursor != p); // XXX
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_cursor = p;
            }
        }

        void set_point(const int32_t x, const int32_t y) {
            set_point(point{x, y});
        }

        template <typename T>
        void set_point(T p) {
            set_point(create_point(p));
        }

        void close_ring() {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Call ring_begin() before add_point()");
            vtzero_assert(m_num_points.value() == 1);
            m_pbf_geometry.add_element(detail::command_close_path(1));
            m_num_points.decrement();
        }

        template <typename TIter>
        void add_ring(TIter begin, TIter end) {
            add_ring(std::distance(begin, end));
            for (; begin != end; ++begin) {
                set_point(create_point(*begin));
            }
        }

        template <typename TIter>
        void add_ring(TIter begin, TIter end, uint32_t count) {
            add_ring(count);
            for (; begin != end; ++begin) {
                set_point(*begin);
#ifndef NDEBUG
                --count;
#endif
            }
            vtzero_assert(count == 0 && "Iterators must yield exactly count points");
        }

        template <typename TContainer>
        void add_ring_from_container(const TContainer& container) {
            add_ring(container.size());
            for (const auto& element : container) {
                set_point(element);
            }
        }

    }; // class polygon_feature_builder

    class geometry_feature_builder : public detail::feature_builder_base {

    public:

        explicit geometry_feature_builder(layer_builder layer) :
            feature_builder_base(&layer.get_layer_impl()) {
        }

        ~geometry_feature_builder() {
            do_commit(); // XXX exceptions?
        }

        geometry_feature_builder(const geometry_feature_builder&) = delete;
        geometry_feature_builder& operator=(const geometry_feature_builder&) = delete;

        geometry_feature_builder(geometry_feature_builder&&) noexcept = default;
        geometry_feature_builder& operator=(geometry_feature_builder&&) noexcept = default;

        void set_id(uint64_t id) {
            vtzero_assert(!m_pbf_tags.valid());
            m_feature_writer.add_uint64(detail::pbf_feature::id, id);
        }

        void set_geometry(const geometry geometry) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(geometry.type()));
            m_feature_writer.add_string(detail::pbf_feature::geometry, geometry.data());
            m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
        }

        template <typename ...TArgs>
        void add_property(TArgs&& ...args) {
            add_property_impl(std::forward<TArgs>(args)...);
        }

    }; // class geometry_feature_builder

    inline void layer_builder::add_feature(const feature& feature) {
        geometry_feature_builder feature_builder{*this};
        if (feature.has_id()) {
            feature_builder.set_id(feature.id());
        }
        feature_builder.set_geometry(feature.geometry());
        feature.for_each_property([&](property&& p) {
            feature_builder.add_property(p);
            return true;
        });
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP
