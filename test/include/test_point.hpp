#ifndef TEST_POINT_HPP
#define TEST_POINT_HPP

/**
 * @file test_point.hpp
 *
 * @brief Contains various point classes used in testing.
 */

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <ostream>

// 2D Point

struct test_point_2d {
    int64_t x = 0;
    int64_t y = 0;

    test_point_2d() = default;

    constexpr test_point_2d(int64_t x_, int64_t y_) noexcept :
        x(x_),
        y(y_) {
   }
};

inline constexpr bool operator==(const test_point_2d lhs, const test_point_2d rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline constexpr bool operator!=(const test_point_2d lhs, const test_point_2d rhs) noexcept {
    return !(lhs == rhs);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const test_point_2d p) {
    return out << '(' << p.x << ',' << p.y << ')';
}

inline vtzero::point create_vtzero_point(test_point_2d p) noexcept {
    return {static_cast<int32_t>(p.x),
            static_cast<int32_t>(p.y)};
}

// 3D Point

struct test_point_3d {
    int64_t x = 0;
    int64_t y = 0;
    int64_t elev = 0;

    constexpr test_point_3d(int64_t x_, int64_t y_, int64_t elev_) noexcept :
        x(x_),
        y(y_),
        elev(elev_) {
    }
};

inline constexpr bool operator==(const test_point_3d& a, const test_point_3d& b) noexcept {
    return a.x == b.x && a.y == b.y && a.elev == b.elev;
}

inline constexpr bool operator!=(const test_point_3d& a, const test_point_3d& b) noexcept {
    return !(a == b);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const test_point_3d& p) {
    return out << '(' << p.x << ',' << p.y << ',' << p.elev << ')';
}

// 3D Point with two attributes

struct test_point_attr {
    int64_t x = 0;
    int64_t y = 0;
    int64_t elev = 0;
    int64_t attr1 = 0;
    int64_t attr2 = 0;

    constexpr test_point_attr(int64_t x_, int64_t y_, int64_t elev_, int64_t a1 = 0, int64_t a2 = 0) noexcept :
        x(x_),
        y(y_),
        elev(elev_),
        attr1(a1),
        attr2(a2) {
    }
};

inline constexpr bool operator==(const test_point_attr& lhs, const test_point_attr& rhs) noexcept {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.elev == rhs.elev &&
           lhs.attr1 == rhs.attr1 &&
           lhs.attr2 == rhs.attr2;
}

inline constexpr bool operator!=(const test_point_attr& lhs, const test_point_attr& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const test_point_attr& p) {
    return out << '(' << p.x << ',' << p.y << ',' << p.elev << ',' << p.attr1 << ',' << p.attr2 << ')';
}

#endif // TEST_POINT_HPP
