#ifndef VTZERO_GEOMETRY_HPP
#define VTZERO_GEOMETRY_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file geometry.hpp
 *
 * @brief Contains classes and functions related to geometry handling.
 */

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_reader.hpp>

#include <cstdint>
#include <utility>

namespace vtzero {

    /// A simple point class
    struct point {

        /// X coordinate
        int32_t x = 0;

        /// Y coordinate
        int32_t y = 0;

        /// Default construct to 0 coordinates
        constexpr point() noexcept = default;

        /// Constructor
        constexpr point(int32_t x_, int32_t y_) noexcept :
            x(x_),
            y(y_) {
        }

    }; // struct point

    /// Helper function to create a point from any type that has members x and y
    template <typename T>
    point create_point(T p) noexcept {
        return {p.x, p.y};
    }

    /// Points are equal if their coordinates are
    inline constexpr bool operator==(const point a, const point b) noexcept {
        return a.x == b.x && a.y == b.y;
    }

    /// Points are not equal if their coordinates aren't
    inline constexpr bool operator!=(const point a, const point b) noexcept {
        return !(a==b);
    }

    namespace detail {

        inline constexpr uint32_t command_integer(const uint32_t id, const uint32_t count) noexcept {
            return (id & 0x7) | (count << 3);
        }

        inline constexpr uint32_t command_move_to(const uint32_t count = 0) noexcept {
            return command_integer(1, count);
        }

        inline constexpr uint32_t command_line_to(const uint32_t count = 0) noexcept {
            return command_integer(2, count);
        }

        inline constexpr uint32_t command_close_path(const uint32_t count = 0) noexcept {
            return command_integer(7, count);
        }

        inline constexpr uint32_t get_command_id(const uint32_t command_integer) noexcept {
            return command_integer & 0x7;
        }

        inline constexpr uint32_t get_command_count(const uint32_t command_integer) noexcept {
            return command_integer >> 3;
        }

        inline constexpr int64_t det(const point a, const point b) noexcept {
            return static_cast<int64_t>(a.x) * static_cast<int64_t>(b.y) -
                   static_cast<int64_t>(b.x) * static_cast<int64_t>(a.y);
        }

        /**
         * Decode a geometry as specified in spec 4.3 from a sequence of 32 bit
         * unsigned integers. This templated base class can be instantiated
         * with a different iterator type for testing than for normal use.
         */
        template <typename TIterator>
        class geometry_decoder {

        public:

            using iterator_type = TIterator;

        private:

            iterator_type m_it;
            iterator_type m_end;

            point m_cursor{0, 0};

            // the last command read
            uint32_t m_command_id = 0;

            /**
             * The current count value is set from the CommandInteger and
             * then counted down with each next_point() call. So it must be
             * greater than 0 when next_point() is called and 0 when
             * next_command() is called.
             */
            uint32_t m_count = 0;

            bool m_strict = true;

        public:

            explicit geometry_decoder(iterator_type begin, iterator_type end, bool strict = true) :
                m_it(begin),
                m_end(end),
                m_strict(strict) {
            }

            uint32_t count() const noexcept {
                return m_count;
            }

            bool done() const noexcept {
                return m_it == m_end;
            }

            bool next_command(const uint32_t expected_command) {
                vtzero_assert(m_count == 0);

                if (m_it == m_end) {
                    return false;
                }

                m_command_id = detail::get_command_id(*m_it);
                if (m_command_id != expected_command) {
                    throw geometry_exception{std::string{"expected command "} +
                                             std::to_string(expected_command) +
                                             " but got " +
                                             std::to_string(m_command_id)};
                }

                if (expected_command == command_close_path()) {
                    // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                    if (detail::get_command_count(*m_it) != 1) {
                        throw geometry_exception{"ClosePath command count is not 1"};
                    }
                } else {
                    m_count = detail::get_command_count(*m_it);
                }

                ++m_it;

                return true;
            }

            point next_point() {
                vtzero_assert(m_count > 0);

                if (m_it == m_end || std::next(m_it) == m_end) {
                    throw geometry_exception{"too few points in geometry"};
                }

                // spec 4.3.2 "A ParameterInteger is zigzag encoded"
                int64_t x = protozero::decode_zigzag32(*m_it++);
                int64_t y = protozero::decode_zigzag32(*m_it++);

                const auto last_cursor = m_cursor;

                // x and y are int64_t so this addition can never overflow
                x += m_cursor.x;
                y += m_cursor.y;

                // The cast is okay, because a valid vector tile can never
                // contain values that would overflow here and we don't care
                // what happens to invalid tiles here.
                m_cursor.x = static_cast<int32_t>(x);
                m_cursor.y = static_cast<int32_t>(y);

                // spec 4.3.3.2 "For any pair of (dX, dY) the dX and dY MUST NOT both be 0."
                if (m_strict && m_command_id == command_line_to() && m_cursor == last_cursor) {
                    throw geometry_exception{"found consecutive equal points (spec 4.3.3.2) (strict mode)"};
                }

                --m_count;

                return m_cursor;
            }

        }; // class geometry_decoder

        template <typename T, typename Enable = void>
        struct get_result {

            using type = void;

            template <typename TGeomHandler>
            void operator()(TGeomHandler&& /*geom_handler*/) const noexcept {
            }

        };

        template <typename T>
        struct get_result<T, typename std::enable_if<!std::is_same<decltype(std::declval<T>().result()), void>::value>::type> {

            using type = decltype(std::declval<T>().result());

            template <typename TGeomHandler>
            type operator()(TGeomHandler&& geom_handler) {
                return std::forward<TGeomHandler>(geom_handler).result();
            }

        };

        template <typename TIterator, typename TGeomHandler>
        typename detail::get_result<TGeomHandler>::type decode_point_geometry(TIterator begin, TIterator end, bool strict, TGeomHandler&& geom_handler) {
            detail::geometry_decoder<TIterator> decoder{begin, end, strict};

            // spec 4.3.4.2 "MUST consist of a single MoveTo command"
            if (!decoder.next_command(detail::command_move_to())) {
                throw geometry_exception{"expected MoveTo command (spec 4.3.4.2)"};
            }

            // spec 4.3.4.2 "command count greater than 0"
            if (decoder.count() == 0) {
                throw geometry_exception{"MoveTo command count is zero (spec 4.3.4.2)"};
            }

            std::forward<TGeomHandler>(geom_handler).points_begin(decoder.count());
            while (decoder.count() > 0) {
                std::forward<TGeomHandler>(geom_handler).points_point(decoder.next_point());
            }

            // spec 4.3.4.2 "MUST consist of of a single ... command"
            if (!decoder.done()) {
                throw geometry_exception{"additional data after end of geometry (spec 4.3.4.2)"};
            }

            std::forward<TGeomHandler>(geom_handler).points_end();

            return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
        }

        template <typename TIterator, typename TGeomHandler>
        typename detail::get_result<TGeomHandler>::type decode_linestring_geometry(TIterator begin, TIterator end, bool strict, TGeomHandler&& geom_handler) {
            detail::geometry_decoder<TIterator> decoder{begin, end, strict};

            // spec 4.3.4.3 "1. A MoveTo command"
            while (decoder.next_command(detail::command_move_to())) {
                // spec 4.3.4.3 "with a command count of 1"
                if (decoder.count() != 1) {
                    throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.3)"};
                }

                const auto first_point = decoder.next_point();

                // spec 4.3.4.3 "2. A LineTo command"
                if (!decoder.next_command(detail::command_line_to())) {
                    throw geometry_exception{"expected LineTo command (spec 4.3.4.3)"};
                }

                // spec 4.3.4.3 "with a command count greater than 0"
                if (decoder.count() == 0) {
                    throw geometry_exception{"LineTo command count is zero (spec 4.3.4.3)"};
                }

                std::forward<TGeomHandler>(geom_handler).linestring_begin(decoder.count() + 1);
                std::forward<TGeomHandler>(geom_handler).linestring_point(first_point);
                while (decoder.count() > 0) {
                    std::forward<TGeomHandler>(geom_handler).linestring_point(decoder.next_point());
                }

                std::forward<TGeomHandler>(geom_handler).linestring_end();
            }

            return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
        }

        template <typename TIterator, typename TGeomHandler>
        typename detail::get_result<TGeomHandler>::type decode_polygon_geometry(TIterator begin, TIterator end, bool strict, TGeomHandler&& geom_handler) {
            detail::geometry_decoder<TIterator> decoder{begin, end, strict};

            // spec 4.3.4.4 "1. A MoveTo command"
            while (decoder.next_command(detail::command_move_to())) {
                // spec 4.3.4.4 "with a command count of 1"
                if (decoder.count() != 1) {
                    throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.4)"};
                }

                int64_t sum = 0;
                const point start_point = decoder.next_point();
                point last_point = start_point;

                // spec 4.3.4.4 "2. A LineTo command"
                if (!decoder.next_command(detail::command_line_to())) {
                    throw geometry_exception{"expected LineTo command (spec 4.3.4.4)"};
                }

                // spec 4.3.4.4 "with a command count greater than 1"
                if (strict && decoder.count() <= 1) {
                    throw geometry_exception{"LineTo command count is not greater than 1 (spec 4.3.4.4)"};
                }

                std::forward<TGeomHandler>(geom_handler).ring_begin(decoder.count() + 2);
                std::forward<TGeomHandler>(geom_handler).ring_point(start_point);

                while (decoder.count() > 0) {
                    const point p = decoder.next_point();
                    sum += detail::det(last_point, p);
                    last_point = p;
                    std::forward<TGeomHandler>(geom_handler).ring_point(p);
                }

                // spec 4.3.4.4 "3. A ClosePath command"
                if (!decoder.next_command(detail::command_close_path())) {
                    throw geometry_exception{"expected ClosePath command (4.3.4.4)"};
                }

                // spec 4.3.4.4 "The position of the cursor before calling the
                // ClosePath command of a linear ring SHALL NOT repeat the same
                // position as the first point in the linear ring"
                if (strict && last_point == start_point) {
                    throw geometry_exception{"duplicate last point of ring"};
                }

                sum += detail::det(last_point, start_point);

                // spec 4.3.4.4 "A linear ring SHOULD NOT have an area ... equal to zero"
                if (strict && sum == 0) {
                    throw geometry_exception{"area of ring is zero"};
                }

                std::forward<TGeomHandler>(geom_handler).ring_point(start_point);

                std::forward<TGeomHandler>(geom_handler).ring_end(sum > 0);
            }

            return detail::get_result<TGeomHandler>{}(std::forward<TGeomHandler>(geom_handler));
        }

    } // namespace detail

    /**
     * Decode a point geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param strict Use strict interpretation of geometry encoding.
     * @param geom_handler An object of TGeomHandler.
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a point geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_point_geometry(const geometry geometry, bool strict, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POINT);
        return detail::decode_point_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a linestring geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param strict Use strict interpretation of geometry encoding.
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a linestring geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_linestring_geometry(const geometry geometry, bool strict, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::LINESTRING);
        return detail::decode_linestring_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a polygon geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param strict Use strict interpretation of geometry encoding.
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a polygon geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_polygon_geometry(const geometry geometry, bool strict, TGeomHandler&& geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POLYGON);
        return detail::decode_polygon_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
    }

    /**
     * Decode a geometry.
     *
     * @tparam TGeomHandler Handler class. See tutorial for details.
     * @param geometry The geometry as returned by feature.geometry().
     * @param strict Use strict interpretation of geometry encoding.
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If the geometry has type UNKNOWN of if there is
     *                        a problem with the geometry.
     */
    template <typename TGeomHandler>
    typename detail::get_result<TGeomHandler>::type decode_geometry(const geometry geometry, bool strict, TGeomHandler&& geom_handler) {
        switch (geometry.type()) {
            case GeomType::POINT:
                return detail::decode_point_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
                break;
            case GeomType::LINESTRING:
                return detail::decode_linestring_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
                break;
            case GeomType::POLYGON:
                return detail::decode_polygon_geometry(geometry.begin(), geometry.end(), strict, std::forward<TGeomHandler>(geom_handler));
                break;
            default:
                break;
        }
        throw geometry_exception{"unknown geometry type"};
    }

} // namespace vtzero

#endif // VTZERO_GEOMETRY_HPP
