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

        constexpr int64_t max_coordinate_epsg3857_cm = 2003750834;

        /**
         * Returns the width or height of a tile in web mercator coordinates
         * for the given zoom level.
         */
        inline constexpr int64_t tile_extent_in_zoom(const uint32_t zoom) noexcept {
            return detail::max_coordinate_epsg3857_cm * 2 / num_tiles_in_zoom(zoom);
        }

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
            vtzero_assert_in_noexcept_function(x < detail::num_tiles_in_zoom(zoom));
            vtzero_assert_in_noexcept_function(y < detail::num_tiles_in_zoom(zoom));
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

        template <typename TPoint>
        TPoint transform(const point p) const noexcept {
            const int64_t e = detail::tile_extent_in_zoom(m_zoom);
            const int64_t x = m_x * e + (p.x * e / m_extent) - detail::max_coordinate_epsg3857_cm;
            const int64_t y = m_y * e + (p.y * e / m_extent) - detail::max_coordinate_epsg3857_cm;
            return {x, y};
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
