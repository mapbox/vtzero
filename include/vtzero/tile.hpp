#ifndef VTZERO_TILE_HPP
#define VTZERO_TILE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file tile.hpp
 *
 * @brief Contains the tile class used for geometry transformations.
 */

#include <vtzero/geometry.hpp>

#include <cstdint>

namespace vtzero {

    namespace detail {

        /**
         * Returns the number of tiles (in each direction) for the given zoom
         * level.
         */
        inline constexpr uint32_t num_tiles_in_zoom(const uint32_t zoom) noexcept {
            return 1u << zoom;
        }

        /// Maximum coordinate value in web mercator in millimeters.
        constexpr const int64_t max_coordinate_epsg3857_mm = 20037508342LL;

    } // namespace detail

    class tile {

        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_zoom;
        uint32_t m_extent;

    public:

        tile(uint32_t x, uint32_t y, uint32_t zoom, uint32_t extent = 4096) noexcept :
            m_x(x),
            m_y(y),
            m_zoom(zoom),
            m_extent(extent) {
            vtzero_assert_in_noexcept_function(zoom < 16 && "zoom out of range");
            vtzero_assert_in_noexcept_function(x < detail::num_tiles_in_zoom(zoom) && "x coordinate out of range");
            vtzero_assert_in_noexcept_function(y < detail::num_tiles_in_zoom(zoom) && "y coordinate out of range");
        }

        uint32_t x() const noexcept {
            return m_x;
        }

        uint32_t y() const noexcept {
            return m_y;
        }

        uint32_t zoom() const noexcept {
            return m_zoom;
        }

        uint32_t extent() const noexcept {
            return m_extent;
        }

        std::pair<int64_t, int64_t> transform_int(const point p) const noexcept {
            const int64_t d = static_cast<int64_t>(detail::num_tiles_in_zoom(m_zoom)) * m_extent;
            const int64_t x = 2 * detail::max_coordinate_epsg3857_mm * (m_extent * m_x  + p.x) / d - detail::max_coordinate_epsg3857_mm;
            const int64_t y = 2 * detail::max_coordinate_epsg3857_mm * (m_extent * m_y  + p.y) / d - detail::max_coordinate_epsg3857_mm;
            return {x, y};
        }

        std::pair<double, double> transform_double(const point p) const noexcept {
            const auto r = transform_int(p);
            return {static_cast<double>(r.first) / 1000,
                    static_cast<double>(r.second) / 1000};
        }

    }; // class tile

    /// Tiles are equal if all their attributes are equal.
    inline bool operator==(const tile lhs, const tile rhs) noexcept {
        return lhs.zoom() == rhs.zoom() &&
               lhs.x() == rhs.x() &&
               lhs.y() == rhs.y();
    }

    /// Tiles are unequal if their are not equal.
    inline bool operator!=(const tile lhs, const tile rhs) noexcept {
        return !(lhs == rhs);
    }

} // namespace vtzero

#endif // VTZERO_TILE_HPP
