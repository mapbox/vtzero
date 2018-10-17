#ifndef VTZERO_GEOMETRY_HPP
#define VTZERO_GEOMETRY_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file geometry.hpp
 *
 * @brief Contains classes and functions related to geometry handling.
 */

#include "attributes.hpp"
#include "exception.hpp"
#include "point.hpp"
#include "types.hpp"
#include "util.hpp"

#include <protozero/pbf_reader.hpp>

#include <cstdint>
#include <limits>
#include <utility>

namespace vtzero {

    /**
     * Type of a polygon ring. This can either be "outer", "inner", or
     * "invalid". Invalid is used when the area of the ring is 0.
     */
    enum class ring_type {
        outer   = 0,
        inner   = 1,
        invalid = 2
    }; // enum class ring_type

    namespace detail {

        /// The command id type as specified in the vector tile spec
        enum class CommandId : uint32_t {
            MOVE_TO    = 1,
            LINE_TO    = 2,
            CLOSE_PATH = 7
        };

        inline constexpr uint32_t command_integer(CommandId id, const uint32_t count) noexcept {
            return (static_cast<uint32_t>(id) & 0x7u) | (count << 3u);
        }

        inline constexpr uint32_t command_move_to(const uint32_t count) noexcept {
            return command_integer(CommandId::MOVE_TO, count);
        }

        inline constexpr uint32_t command_line_to(const uint32_t count) noexcept {
            return command_integer(CommandId::LINE_TO, count);
        }

        inline constexpr uint32_t command_close_path() noexcept {
            return command_integer(CommandId::CLOSE_PATH, 1);
        }

        inline constexpr uint32_t get_command_id(const uint32_t command_integer) noexcept {
            return command_integer & 0x7u;
        }

        inline constexpr uint32_t get_command_count(const uint32_t command_integer) noexcept {
            return command_integer >> 3u;
        }

        // The maximum value for the command count according to the spec.
        inline constexpr uint32_t max_command_count() noexcept {
            return get_command_count(std::numeric_limits<uint32_t>::max());
        }

        // The null_iterator simply does nothing but has a valid iterator
        // interface. It is used for simple 2D geometries without geometric
        // attributes and for testing.
        template <typename T>
        struct null_iterator {

            constexpr bool operator==(null_iterator /*other*/) const noexcept {
                return true;
            }

            constexpr bool operator!=(null_iterator /*other*/) const noexcept {
                return false;
            }

            constexpr T operator*() const noexcept {
                return 0;
            }

            constexpr null_iterator operator++() const noexcept {
                return *this;
            }

            constexpr null_iterator operator++(int) const noexcept {
                return *this;
            }

        }; // struct null_iterator

        using dummy_elev_iterator = null_iterator<int32_t>;
        using dummy_attr_iterator = null_iterator<uint64_t>;

        template <typename TIterator>
        class geometric_attribute {

            TIterator m_it{};
            index_value m_key_index{};
            index_value m_scaling_index{};
            uint64_t m_count = 0;
            int64_t m_value = 0;

        public:

            geometric_attribute() = default;

            geometric_attribute(TIterator it, uint64_t key_index, uint64_t scaling_index, uint64_t count) :
                m_it(it),
                m_key_index(static_cast<uint32_t>(key_index)),
                m_scaling_index(static_cast<uint32_t>(scaling_index)),
                m_count(count) {
            }

            index_value key_index() const noexcept {
                return m_key_index;
            }

            index_value scaling_index() const noexcept {
                return m_scaling_index;
            }

            bool get_next_value() noexcept {
                if (m_count == 0) {
                    return false;
                }
                const uint64_t raw_value = *m_it++;
                --m_count;
                if (raw_value == 0) {
                    return false;
                }
                m_value += protozero::decode_zigzag64(raw_value - 1);
                return true;
            }

            int64_t value() const noexcept {
                return m_value;
            }

        }; // class geometric_attribute

        template <>
        class geometric_attribute<dummy_attr_iterator> {

        public:

            index_value key_index() const noexcept {
                return {};
            }

            index_value scaling_index() const noexcept {
                return {};
            }

            bool get_next_value() noexcept {
                return false;
            }

            int64_t value() const noexcept {
                return 0;
            }

        }; // class geometric_attribute

        template <unsigned int MaxGeometricAttributes, typename TIterator>
        class geometric_attribute_collection {

            static_assert(!std::is_same<TIterator, dummy_attr_iterator>::value,
                          "Please set MaxGeometricAttributes to 0 when dummy_attr_iterator is used");

            geometric_attribute<TIterator> m_attrs[MaxGeometricAttributes];

            std::size_t m_size = 0;

        public:

            geometric_attribute_collection(TIterator it, const TIterator end) :
                m_attrs() {
                while (it != end && m_size < MaxGeometricAttributes) {
                    const uint64_t complex_value = *it++;
                    if ((complex_value & 0xfu) == static_cast<uint64_t>(complex_value_type::cvt_number_list)) {
                        if (it == end) {
                            throw format_exception{"geometric attributes end too soon"};
                        }

                        auto attr_count = *it++;
                        if (it == end) {
                            throw format_exception{"geometric attributes end too soon"};
                        }
                        const uint64_t scaling = *it++;
                        if (it == end) {
                            throw format_exception{"geometric attributes end too soon"};
                        }

                        m_attrs[m_size] = {it, complex_value >> 4u, scaling, attr_count};
                        ++m_size;

                        while (attr_count > 0) {
                            --attr_count;
                            ++it;
                            if (attr_count != 0 && it == end) {
                                throw format_exception{"geometric attributes end too soon"};
                            }
                        }
                    } else if ((complex_value & 0xfu) == static_cast<uint64_t>(complex_value_type::cvt_list)) {
                        throw format_exception{"geometric attributes of type 'list' not implemented yet"}; // XXX
                    } else {
                        throw format_exception{"geometric attributes must be of type 'list' or 'number list'"};
                    }
                }
            }

            geometric_attribute<TIterator>* begin() noexcept {
                return m_attrs;
            }

            geometric_attribute<TIterator>* end() noexcept {
                return m_attrs + m_size;
            }

        }; // class geometric_attribute_collection

        template <typename TIterator>
        class geometric_attribute_collection<0, TIterator> {

        public:

            geometric_attribute_collection(TIterator /*it*/, const TIterator /*end*/) noexcept {
            }

            geometric_attribute<TIterator>* begin() const noexcept {
                return nullptr;
            }

            geometric_attribute<TIterator>* end() const noexcept {
                return nullptr;
            }

        }; // class geometric_attribute_collection<0, TIterator>

        /**
         * Decode a geometry as specified in spec 4.3. This templated class can
         * be instantiated with a different iterator type for testing than for
         * normal use.
         */
        template <int Dimensions,
                  unsigned int MaxGeometricAttributes,
                  typename TGeomIterator,
                  typename TElevIterator = dummy_elev_iterator,
                  typename TAttrIterator = dummy_attr_iterator>
        class geometry_decoder {

            static_assert(Dimensions == 2 || Dimensions == 3, "Need 2 or 3 dimensions");

            static inline constexpr int64_t det(const point<Dimensions>& a, const point<Dimensions>& b) noexcept {
                return static_cast<int64_t>(a.x) * static_cast<int64_t>(b.y) -
                       static_cast<int64_t>(b.x) * static_cast<int64_t>(a.y);
            }

            TGeomIterator m_geom_it;
            TGeomIterator m_geom_end;

            TElevIterator m_elev_it;
            TElevIterator m_elev_end;

            TAttrIterator m_attr_it;
            TAttrIterator m_attr_end;

            point<Dimensions> m_cursor;

            // maximum value for m_count before we throw an exception
            uint32_t m_max_count;

            /**
             * The current count value is set from the CommandInteger and
             * then counted down with each next_point() call. So it must be
             * greater than 0 when next_point() is called and 0 when
             * next_command() is called.
             */
            uint32_t m_count = 0;

        public:

            geometry_decoder(std::size_t max,
                             TGeomIterator geom_begin, TGeomIterator geom_end,
                             TElevIterator elev_begin = TElevIterator{}, TElevIterator elev_end = TElevIterator{},
                             TAttrIterator attr_begin = TAttrIterator{}, TAttrIterator attr_end = TAttrIterator{}) :
                m_geom_it(geom_begin),
                m_geom_end(geom_end),
                m_elev_it(elev_begin),
                m_elev_end(elev_end),
                m_attr_it(attr_begin),
                m_attr_end(attr_end),
                m_max_count(static_cast<uint32_t>(max)) {
                vtzero_assert(max <= max_command_count());
            }

            uint32_t count() const noexcept {
                return m_count;
            }

            bool done() const noexcept {
                return m_geom_it == m_geom_end &&
                       (Dimensions == 2 || m_elev_it == m_elev_end);
            }

            bool next_command(const CommandId expected_command_id) {
                vtzero_assert(m_count == 0);

                if (m_geom_it == m_geom_end) {
                    return false;
                }

                const auto command_id = get_command_id(*m_geom_it);
                if (command_id != static_cast<uint32_t>(expected_command_id)) {
                    throw geometry_exception{std::string{"expected command "} +
                                             std::to_string(static_cast<uint32_t>(expected_command_id)) +
                                             " but got " +
                                             std::to_string(command_id)};
                }

                if (expected_command_id == CommandId::CLOSE_PATH) {
                    // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                    if (get_command_count(*m_geom_it) != 1) {
                        throw geometry_exception{"ClosePath command count is not 1"};
                    }
                } else {
                    m_count = get_command_count(*m_geom_it);
                    if (m_count > m_max_count) {
                        throw geometry_exception{"count too large"};
                    }
                }

                ++m_geom_it;

                return true;
            }

            point<Dimensions> next_point() {
                vtzero_assert(m_count > 0);

                if (m_geom_it == m_geom_end || std::next(m_geom_it) == m_geom_end) {
                    throw geometry_exception{"too few points in geometry"};
                }

                // spec 4.3.2 "A ParameterInteger is zigzag encoded"
                int64_t x = protozero::decode_zigzag32(*m_geom_it++);
                int64_t y = protozero::decode_zigzag32(*m_geom_it++);

                // x and y are int64_t so this addition can never overflow.
                // If we did this calculation directly with int32_t we'd get
                // a possible integer overflow.
                x += m_cursor.x;
                y += m_cursor.y;

                // The cast is okay, because a valid vector tile can never
                // contain values that would overflow here and we don't care
                // what happens to invalid tiles here.
                m_cursor.x = static_cast<int32_t>(x);
                m_cursor.y = static_cast<int32_t>(y);

                if (Dimensions == 3 && m_elev_it != m_elev_end) {
                    m_cursor.add_to_z(*m_elev_it++);
                }

                --m_count;

                return m_cursor;
            }

            template <typename THandler>
            get_result_t<THandler> decode_point(THandler&& handler) {
                using handler_type = typename std::remove_reference<THandler>::type;

                // spec 4.3.4.2 "MUST consist of a single MoveTo command"
                if (!next_command(CommandId::MOVE_TO)) {
                    throw geometry_exception{"expected MoveTo command (spec 4.3.4.2)"};
                }

                // spec 4.3.4.2 "command count greater than 0"
                if (count() == 0) {
                    throw geometry_exception{"MoveTo command count is zero (spec 4.3.4.2)"};
                }

                geometric_attribute_collection<MaxGeometricAttributes, TAttrIterator> geom_attributes{m_attr_it, m_attr_end};

                std::forward<THandler>(handler).points_begin(count());
                while (count() > 0) {
                    std::forward<THandler>(handler).points_point(handler_type::convert(next_point()));
                    for (auto& attr : geom_attributes) {
                        if (attr.get_next_value()) {
                            call_points_attr(std::forward<THandler>(handler), attr.key_index(), attr.scaling_index(), attr.value());
                        } else {
                            call_points_null_attr(std::forward<THandler>(handler), attr.key_index());
                        }
                    }
                }

                // spec 4.3.4.2 "MUST consist of a single ... command"
                if (!done()) {
                    throw geometry_exception{"additional data after end of geometry (spec 4.3.4.2)"};
                }

                std::forward<THandler>(handler).points_end();

                return get_result<THandler>::of(std::forward<THandler>(handler));
            }

            template <typename THandler>
            get_result_t<THandler> decode_linestring(THandler&& handler) {
                using handler_type = typename std::remove_reference<THandler>::type;

                geometric_attribute_collection<MaxGeometricAttributes, TAttrIterator> geom_attributes{m_attr_it, m_attr_end};

                // spec 4.3.4.3 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.3 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.3)"};
                    }

                    auto point = next_point();

                    // spec 4.3.4.3 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.3)"};
                    }

                    // spec 4.3.4.3 "with a command count greater than 0"
                    if (count() == 0) {
                        throw geometry_exception{"LineTo command count is zero (spec 4.3.4.3)"};
                    }

                    std::forward<THandler>(handler).linestring_begin(count() + 1);

                    while (true) {
                        std::forward<THandler>(handler).linestring_point(handler_type::convert(point));
                        for (auto& attr : geom_attributes) {
                            if (attr.get_next_value()) {
                                call_points_attr(std::forward<THandler>(handler), attr.key_index(), attr.scaling_index(), attr.value());
                            } else {
                                call_points_null_attr(std::forward<THandler>(handler), attr.key_index());
                            }
                        }
                        if (count() == 0) {
                            break;
                        }
                        point = next_point();
                    }

                    std::forward<THandler>(handler).linestring_end();
                }

                return get_result<THandler>::of(std::forward<THandler>(handler));
            }

            template <typename THandler>
            get_result_t<THandler> decode_polygon(THandler&& handler) {
                using handler_type = typename std::remove_reference<THandler>::type;

                geometric_attribute_collection<MaxGeometricAttributes, TAttrIterator> geom_attributes{m_attr_it, m_attr_end};

                // spec 4.3.4.4 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.4 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.4)"};
                    }

                    int64_t sum = 0;
                    const auto start_point = next_point();
                    auto point = start_point;

                    // spec 4.3.4.4 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.4)"};
                    }

                    std::forward<THandler>(handler).ring_begin(count() + 2);

                    while (true) {
                        std::forward<THandler>(handler).ring_point(handler_type::convert(point));
                        for (auto& attr : geom_attributes) {
                            if (attr.get_next_value()) {
                                call_points_attr(std::forward<THandler>(handler), attr.key_index(), attr.scaling_index(), attr.value());
                            } else {
                                call_points_null_attr(std::forward<THandler>(handler), attr.key_index());
                            }
                        }

                        if (count() == 0) {
                            break;
                        }

                        const auto new_point = next_point();
                        sum += det(point, new_point);
                        point = new_point;
                    }

                    // spec 4.3.4.4 "3. A ClosePath command"
                    if (!next_command(CommandId::CLOSE_PATH)) {
                        throw geometry_exception{"expected ClosePath command (4.3.4.4)"};
                    }

                    sum += det(point, start_point);

                    std::forward<THandler>(handler).ring_point(handler_type::convert(start_point));
                    for (auto& attr : geom_attributes) {
                        if (attr.get_next_value()) {
                            call_points_attr(std::forward<THandler>(handler), attr.key_index(), attr.scaling_index(), attr.value());
                        } else {
                            call_points_null_attr(std::forward<THandler>(handler), attr.key_index());
                        }
                    }

                    std::forward<THandler>(handler).ring_end(sum > 0 ? ring_type::outer :
                                                             sum < 0 ? ring_type::inner : ring_type::invalid);
                }

                return get_result<THandler>::of(std::forward<THandler>(handler));
            }

        }; // class geometry_decoder

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_GEOMETRY_HPP
