#ifndef VTZERO_POINT_HPP
#define VTZERO_POINT_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file point.hpp
 *
 * @brief Contains the point template class.
 */

#include <cstdint>

namespace vtzero {

    /// The vtzero point class
    template <int Dimensions>
    struct point {
        static_assert(Dimensions == 2 || Dimensions == 3, "point class can only be instantiated with 2 or 3 dimensions");
    };

    /// Points are not equal if their coordinates aren't.
    template <int Dimensions>
    inline constexpr bool operator!=(const point<Dimensions> a, const point<Dimensions> b) noexcept {
        return !(a == b);
    }

    /// Specialization of the vtzero point class for 2 dimensions
    template <>
    struct point<2> {

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

        /// Get z value (dummy function for 2d coordinates)
        static int32_t get_z() noexcept {
            return 0;
        }

        /// Add value to z value (dummy function for 2d coordinates)
        void add_to_z(int32_t /*value*/) const noexcept {
        }

        /// Set z value (dummy function for 2d coordinates)
        void set_z(int32_t /*value*/) const noexcept {
        }

    }; // struct point

    /// Points are equal if their coordinates are.
    inline constexpr bool operator==(const point<2> a, const point<2> b) noexcept {
        return a.x == b.x && a.y == b.y;
    }

    /// Specialization of the vtzero point class for 3 dimensions
    template <>
    struct point<3> {

        /// X coordinate
        int32_t x = 0;

        /// Y coordinate
        int32_t y = 0;

        /// elevation
        int32_t z = 0;

        /// Default construct to 0 coordinates
        constexpr point<3>() noexcept = default;

        /// Constructor
        constexpr point<3>(int32_t x_, int32_t y_, int32_t z_ = 0) noexcept :
            x(x_),
            y(y_),
            z(z_) {
        }

        /// Get z value
        int32_t get_z() const noexcept {
            return z;
        }

        /// Add value to z value
        void add_to_z(int32_t value) noexcept {
            z = static_cast<int32_t>(static_cast<int64_t>(z) + static_cast<int64_t>(value));
        }

        /// Set z value
        void set_z(int32_t value) noexcept {
            z = value;
        }

    }; // struct point

    /// Points are equal if their coordinates are.
    inline constexpr bool operator==(const point<3>& a, const point<3>& b) noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    /// Alias for 2d points
    using point_2d = point<2>;

    /// Alias for 3d points
    using point_3d = point<3>;

} // namespace vtzero

#endif // VTZERO_POINT_HPP
