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
#include <utility>

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

    /**
     * This class repesents a tile, it doesn't contain any data, just the
     * zoom level, x and y coordinates and the extent. The class is used to
     * convert tile coordinates into web mercator coordinates.
     */
    class tile {

        uint32_t m_x    = 0;
        uint32_t m_y    = 0;
        uint32_t m_zoom = 0;

        uint32_t m_extent = 0;

    public:

        /// The zoom level in a tile must be smaller than this.
        enum : uint32_t {
            max_zoom = 32
        };

        tile() noexcept = default;

        /**
         * Construct a tile.
         *
         * @param x X coordinate of the tile (0 <= x < 2^zoom)
         * @param y Y coordinate of the tile (0 <= y < 2^zoom)
         * @param zoom Zoom level of the tile (0 <= zoom < max_zoom)
         * @param extent Extent used in vector tiles
         */
        tile(uint32_t x, uint32_t y, uint32_t zoom, uint32_t extent = 4096) noexcept :
            m_x(x),
            m_y(y),
            m_zoom(zoom),
            m_extent(extent) {
            vtzero_assert_in_noexcept_function(zoom < max_zoom && "zoom out of range");
            vtzero_assert_in_noexcept_function(x < detail::num_tiles_in_zoom(zoom) && "x coordinate out of range");
            vtzero_assert_in_noexcept_function(y < detail::num_tiles_in_zoom(zoom) && "y coordinate out of range");
            vtzero_assert_in_noexcept_function(extent != 0 && "extent can not be 0");
        }

        /// Is this a valid (non-default-constructed) tile.
        bool valid() const noexcept {
            return m_extent != 0;
        }

        /**
         * The x coordinate.
         *
         * Always returns 0 for invalid tiles.
         */
        constexpr uint32_t x() const noexcept {
            return m_x;
        }

        /**
         * The y coordinate.
         *
         * Always returns 0 for invalid tiles.
         */
        constexpr uint32_t y() const noexcept {
            return m_y;
        }

        /**
         * The zoom level.
         *
         * Always returns 0 for invalid tiles.
         */
        constexpr uint32_t zoom() const noexcept {
            return m_zoom;
        }

        /**
         * The extent.
         *
         * Always returns 0 for invalid tiles.
         */
        constexpr uint32_t extent() const noexcept {
            return m_extent;
        }

        /**
         * Transform the vector tile coordinates specified as argument into
         * web mercator coordinates
         *
         * This uses integer arithmetic only.
         *
         * @param p Vector tile coordinates
         * @returns coordinate pair of web mercator coordinates (in millimeters).
         * @pre valid()
         */
        std::pair<int64_t, int64_t> transform_int(const point_2d p) const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            const int64_t d = static_cast<int64_t>(detail::num_tiles_in_zoom(m_zoom)) * m_extent;
            const int64_t x = 2 * detail::max_coordinate_epsg3857_mm * (static_cast<int64_t>(m_extent * m_x) + p.x) / d - detail::max_coordinate_epsg3857_mm;
            const int64_t y = 2 * detail::max_coordinate_epsg3857_mm * (static_cast<int64_t>(m_extent * m_y) + p.y) / d - detail::max_coordinate_epsg3857_mm;
            return {x, y};
        }

        /**
         * Transform the vector tile coordinates specified as argument into
         * web mercator coordinates
         *
         * @param p Vector tile coordinates
         * @returns coordinate pair of web mercator coordinates (in meters).
         * @pre valid()
         */
        std::pair<double, double> transform_double(const point_2d p) const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            const auto r = transform_int(p);
            return {static_cast<double>(r.first) / 1000,
                    static_cast<double>(r.second) / 1000};
        }

    }; // class tile

    /// Tiles are equal if all their attributes are equal.
    inline bool operator==(const tile lhs, const tile rhs) noexcept {
        return lhs.zoom() == rhs.zoom() &&
               lhs.x() == rhs.x() &&
               lhs.y() == rhs.y() &&
               lhs.extent() == rhs.extent();
    }

    /// Tiles are unequal if they are not equal.
    inline bool operator!=(const tile lhs, const tile rhs) noexcept {
        return !(lhs == rhs);
    }

} // namespace vtzero

#endif // VTZERO_TILE_HPP
