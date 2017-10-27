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

#include "encoded_property_value.hpp"
#include "geometry.hpp"
#include "types.hpp"
#include "vector_tile.hpp"

#include <protozero/pbf_builder.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vtzero {

    class layer_builder_base {

    public:

        layer_builder_base() noexcept = default;

        virtual ~layer_builder_base() noexcept = default;

        layer_builder_base(const layer_builder_base&) noexcept = default;
        layer_builder_base& operator=(const layer_builder_base&) noexcept = default;

        layer_builder_base(layer_builder_base&&) noexcept = default;
        layer_builder_base& operator=(layer_builder_base&&) noexcept = default;

        virtual void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const = 0;

    }; // class layer_builder_base

    class layer_builder_impl : public layer_builder_base {

        std::string m_data;
        std::string m_keys_data;
        std::string m_values_data;

        protozero::pbf_builder<detail::pbf_layer> m_pbf_message_layer;
        protozero::pbf_builder<detail::pbf_layer> m_pbf_message_keys;
        protozero::pbf_builder<detail::pbf_layer> m_pbf_message_values;

        std::size_t m_num_features = 0;
        uint32_t m_max_key = 0;
        uint32_t m_max_value = 0;

        static index_value find_in_table(const data_view text, const std::string& data) {
            uint32_t index = 0;
            protozero::pbf_message<detail::pbf_layer> pbf_table{data};

            while (pbf_table.next()) {
                const auto v = pbf_table.get_view();
                if (v == text) {
                    return index_value{index};
                }
                ++index;
            }

            return index_value{};
        }

    public:

        template <typename T>
        layer_builder_impl(T&& name, uint32_t version, uint32_t extent) :
            m_pbf_message_layer(m_data),
            m_pbf_message_keys(m_keys_data),
            m_pbf_message_values(m_values_data) {
            m_pbf_message_layer.add_uint32(detail::pbf_layer::version, version);
            m_pbf_message_layer.add_string(detail::pbf_layer::name, std::forward<T>(name));
            m_pbf_message_layer.add_uint32(detail::pbf_layer::extent, extent);
        }

        ~layer_builder_impl() noexcept override = default;

        layer_builder_impl(const layer_builder_impl&) = delete;
        layer_builder_impl& operator=(const layer_builder_impl&) = delete;

        layer_builder_impl(layer_builder_impl&&) = default;
        layer_builder_impl& operator=(layer_builder_impl&&) = default;

        index_value add_key_without_dup_check(const data_view text) {
            m_pbf_message_keys.add_string(detail::pbf_layer::keys, text);
            return m_max_key++;
        }

        index_value add_key(const data_view text) {
            const auto index = find_in_table(text, m_keys_data);
            if (index.valid()) {
                return index;
            }
            return add_key_without_dup_check(text);
        }

        index_value add_value_without_dup_check(const data_view text) {
            m_pbf_message_values.add_string(detail::pbf_layer::values, text);
            return m_max_value++;
        }

        index_value add_value(const data_view text) {
            const auto index = find_in_table(text, m_values_data);
            if (index.valid()) {
                return index;
            }
            return add_value_without_dup_check(text);
        }

        const std::string& data() const noexcept {
            return m_data;
        }

        const std::string& keys_data() const noexcept {
            return m_keys_data;
        }

        const std::string& values_data() const noexcept {
            return m_values_data;
        }

        protozero::pbf_builder<detail::pbf_layer>& message() noexcept {
            return m_pbf_message_layer;
        }

        void increment_feature_count() noexcept {
            ++m_num_features;
        }

        void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const override {
            if (m_num_features > 0) {
                pbf_tile_builder.add_bytes_vectored(detail::pbf_tile::layers,
                    data(),
                    keys_data(),
                    values_data()
                );
            }
        }

    }; // class layer_builder_impl

    class layer_builder_existing : public layer_builder_base {

        data_view m_data;

    public:

        explicit layer_builder_existing(const data_view data) :
            m_data(data) {
        }

        void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const override {
            pbf_tile_builder.add_bytes(detail::pbf_tile::layers, m_data);
        }

    }; // class layer_builder_existing

    class tile_builder;

    class layer_builder {

        vtzero::layer_builder_impl* m_layer;

    public:

        template <typename ...TArgs>
        explicit layer_builder(vtzero::tile_builder& tile, TArgs&& ...args);

        vtzero::layer_builder_impl& get_layer() noexcept {
            return *m_layer;
        };

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

        void add_feature(const feature& feature);

    }; // class layer_builder

    namespace detail {

        class feature_builder_base {

            layer_builder m_layer;

            void add_key_internal(index_value idx) {
                m_pbf_tags.add_element(idx.value());
            }

            template <typename T>
            void add_key_internal(T&& key) {
                add_key_internal(m_layer.get_layer().add_key(data_view{std::forward<T>(key)}));
            }

            void add_value_internal(index_value idx) {
                m_pbf_tags.add_element(idx.value());
            }

            void add_value_internal(property_value value) {
                add_value_internal(m_layer.get_layer().add_value(value.data()));
            }

            template <typename T>
            void add_value_internal(T&& value) {
                encoded_property_value v{std::forward<T>(value)};
                add_value_internal(m_layer.get_layer().add_value(v.data()));
            }

        protected:

            protozero::pbf_builder<detail::pbf_feature> m_feature_writer;
            protozero::packed_field_uint32 m_pbf_tags;

            feature_builder_base(layer_builder layer, uint64_t id) :
                m_layer(layer),
                m_feature_writer(layer.get_layer().message(), detail::pbf_layer::features) {
                m_feature_writer.add_uint64(detail::pbf_feature::id, id);
            }

            ~feature_builder_base() noexcept = default;

            feature_builder_base(const feature_builder_base&) = delete; // NOLINT clang-tidy: hicpp-use-equals-delete
            feature_builder_base& operator=(const feature_builder_base&) = delete; // NOLINT clang-tidy: hicpp-use-equals-delete
                                                                                   // The check wants these functions to be public...

            feature_builder_base(feature_builder_base&&) noexcept = default;
            feature_builder_base& operator=(feature_builder_base&&) noexcept = default;

            void add_property_impl(const property& property) {
                add_key_internal(property.key());
                add_value_internal(property.value());
            }

            template <typename TKey, typename TValue>
            void add_property_impl(TKey&& key, TValue&& value) {
                add_key_internal(std::forward<TKey>(key));
                add_value_internal(std::forward<TValue>(value));
            }

            void do_commit() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                if (m_feature_writer.valid()) {
                    m_feature_writer.commit();
                    m_layer.get_layer().increment_feature_count();
                }
            }

            void do_rollback() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                m_feature_writer.rollback();
            }

        }; // class feature_builder_base

        class feature_builder : public feature_builder_base {

        protected:

            protozero::packed_field_uint32 m_pbf_geometry{};
            uint32_t m_num_points = 0;
            point m_cursor{0, 0};

        public:

            feature_builder(layer_builder layer, uint64_t id) :
                feature_builder_base(layer, id) {
            }

            ~feature_builder() {
                commit(); // XXX exceptions?
            }

            feature_builder(const feature_builder&) = delete;
            feature_builder& operator=(const feature_builder&) = delete;

            feature_builder(feature_builder&&) noexcept = default;
            feature_builder& operator=(feature_builder&&) noexcept = default;

            template <typename ...TArgs>
            void add_property(TArgs&& ...args) {
                if (m_pbf_geometry.valid()) {
                    vtzero_assert(m_num_points == 0 && "Not enough calls to set_point()");
                    m_pbf_geometry.commit();
                }
                if (!m_pbf_tags.valid()) {
                    m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
                }
                add_property_impl(std::forward<TArgs>(args)...);
            }

            void commit() {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                }
                do_commit();
            }

            void rollback() {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                }
                do_rollback();
            }

        }; // class feature_builder

    } // namespace detail

    class geometry_feature_builder : public detail::feature_builder_base {

    public:

        geometry_feature_builder(layer_builder layer, const geometry geometry, uint64_t id = 0) :
            feature_builder_base(layer, id) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(geometry.type()));
            m_feature_writer.add_string(detail::pbf_feature::geometry, geometry.data());
            m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
        }

        ~geometry_feature_builder() {
            do_commit(); // XXX exceptions?
        }

        geometry_feature_builder(const geometry_feature_builder&) = delete;
        geometry_feature_builder& operator=(const geometry_feature_builder&) = delete;

        geometry_feature_builder(geometry_feature_builder&&) noexcept = default;
        geometry_feature_builder& operator=(geometry_feature_builder&&) noexcept = default;

        template <typename ...TArgs>
        void add_property(TArgs&& ...args) {
            add_property_impl(std::forward<TArgs>(args)...);
        }

    }; // class geometry_feature_builder

    class point_feature_builder : public detail::feature_builder {

    public:

        explicit point_feature_builder(layer_builder layer, uint64_t id = 0) :
            feature_builder(layer, id) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POINT));
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
        }

        ~point_feature_builder() {
            vtzero_assert_in_noexcept_function(m_num_points == 0 && "has fewer points than expected");
        }

        point_feature_builder(const point_feature_builder&) = delete;
        point_feature_builder& operator=(const point_feature_builder&) = delete;

        point_feature_builder(point_feature_builder&&) noexcept = default;
        point_feature_builder& operator=(point_feature_builder&&) noexcept = default;

        void add_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            m_pbf_geometry.add_element(detail::command_move_to(1));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y));
            m_pbf_geometry.commit();
        }

        void add_point(const int32_t x, const int32_t y) {
            add_point(point{x, y});
        }

        template <typename T>
        void add_point(T p) {
            add_point(create_point(p));
        }

        void add_points(uint32_t count) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 0);
            m_num_points = count;
            m_pbf_geometry.add_element(detail::command_move_to(count));
        }

        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Call add_points() before set_point()");
            vtzero_assert(m_num_points > 0 && "Too many points");
            --m_num_points;
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
            m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
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
        void add_points(TIter begin, TIter end) {
            add_points(std::distance(begin, end));
            for (; begin != end; ++begin) {
                set_point(*begin);
            }
            m_pbf_geometry.commit();
        }

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

        template <typename TContainer>
        void add_points_from_container(const TContainer& container) {
            add_points(container.size());
            for (const auto& element : container) {
                set_point(element);
            }
            m_pbf_geometry.commit();
        }

    }; // class point_feature_builder

    class line_string_feature_builder : public detail::feature_builder {

        bool m_start_line = false;

    public:

        explicit line_string_feature_builder(layer_builder layer, uint64_t id = 0) :
            feature_builder(layer, id) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::LINESTRING));
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
        }

        ~line_string_feature_builder() {
            vtzero_assert_in_noexcept_function(m_num_points == 0 && "LineString has fewer points than expected");
        }

        line_string_feature_builder(const line_string_feature_builder&) = delete;
        line_string_feature_builder& operator=(const line_string_feature_builder&) = delete;

        line_string_feature_builder(line_string_feature_builder&&) noexcept = default;
        line_string_feature_builder& operator=(line_string_feature_builder&&) noexcept = default;

        void add_linestring(const uint32_t count) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 1);
            vtzero_assert(m_num_points == 0 && "LineString has fewer points than expected");
            m_num_points = count;
            m_start_line = true;
        }

        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Add full geometry before adding tags");
            vtzero_assert(m_num_points > 0 && "Too many calls to set_point()");
            --m_num_points;
            if (m_start_line) {
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points));
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

    }; // class line_string_feature_builder

    class polygon_feature_builder : public detail::feature_builder {

        point m_first_point{0, 0};
        bool m_start_ring = false;

    public:

        explicit polygon_feature_builder(layer_builder layer, uint64_t id = 0) :
            feature_builder(layer, id) {
            m_feature_writer.add_enum(detail::pbf_feature::type, static_cast<int32_t>(GeomType::POLYGON));
            m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
        }

        ~polygon_feature_builder() {
            vtzero_assert_in_noexcept_function(m_num_points == 0 && "ring has fewer points than expected");
        }

        polygon_feature_builder(const polygon_feature_builder&) = delete;
        polygon_feature_builder& operator=(const polygon_feature_builder&) = delete;

        polygon_feature_builder(polygon_feature_builder&&) noexcept = default;
        polygon_feature_builder& operator=(polygon_feature_builder&&) noexcept = default;

        void add_ring(const uint32_t count) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid());
            vtzero_assert(count > 3);
            vtzero_assert(m_num_points == 0 && "ring has fewer points than expected");
            m_num_points = count;
            m_start_ring = true;
        }

        void set_point(const point p) {
            vtzero_assert(m_pbf_geometry.valid());
            vtzero_assert(!m_pbf_tags.valid() && "Call ring_begin() before add_point()");
            vtzero_assert(m_num_points > 0 && "Too many calls to add_point()");
            --m_num_points;
            if (m_start_ring) {
                m_first_point = p;
                m_pbf_geometry.add_element(detail::command_move_to(1));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.x - m_cursor.x));
                m_pbf_geometry.add_element(protozero::encode_zigzag32(p.y - m_cursor.y));
                m_pbf_geometry.add_element(detail::command_line_to(m_num_points - 1));
                m_start_ring = false;
                m_cursor = m_first_point;
            } else if (m_num_points == 0) {
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
            vtzero_assert(m_num_points == 1);
            m_pbf_geometry.add_element(detail::command_close_path(1));
            --m_num_points;
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

    class tile_builder {

        std::vector<std::unique_ptr<layer_builder_base>> m_layers;

    public:

        tile_builder() = default;

        ~tile_builder() noexcept = default;

        tile_builder(const tile_builder&) = delete;
        tile_builder& operator=(const tile_builder&) = delete;

        tile_builder(tile_builder&&) = default;
        tile_builder& operator=(tile_builder&&) = default;

        layer_builder_impl* add_layer(layer& layer) {
            const auto ptr = new layer_builder_impl{layer.name(), layer.version(), layer.extent()};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        template <typename T>
        layer_builder_impl* add_layer(T&& name, uint32_t version = 2, uint32_t extent = 4096) {
            const auto ptr = new layer_builder_impl{std::forward<T>(name), version, extent};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        void add_layer(data_view&& data) {
            m_layers.emplace_back(new layer_builder_existing{std::forward<data_view>(data)});
        }

        void serialize(std::string& data) const {
            protozero::pbf_builder<detail::pbf_tile> pbf_tile_builder{data};
            for (const auto& layer : m_layers) {
                layer->build(pbf_tile_builder);
            }
        }

        std::string serialize() const {
            std::string data;
            serialize(data);
            return data;
        }

    }; // class tile_builder

    inline void layer_builder::add_feature(const feature& feature) {
        geometry_feature_builder feature_builder{*this, feature.geometry(), feature.id()};
        feature.for_each_property([&](property&& p) {
            feature_builder.add_property(p);
            return true;
        });
    }

    template <typename ...TArgs>
    layer_builder::layer_builder(vtzero::tile_builder& tile, TArgs&& ...args) :
        m_layer(tile.add_layer(std::forward<TArgs>(args)...)) {
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HPP
