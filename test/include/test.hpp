#ifndef TEST_HPP
#define TEST_HPP

#include <catch.hpp>

#include <stdexcept>

// Define vtzero_assert() to throw this error. This allows the tests to
// check that the assert fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};

#define vtzero_assert(x) if (!(x)) { throw assert_error{#x}; }

#define vtzero_assert_in_noexcept_function(x) if (!(x)) { got_an_assert = true; }

extern bool got_an_assert;

#define REQUIRE_ASSERT(x) x; REQUIRE(got_an_assert); got_an_assert = false;

#include <vtzero/output.hpp>

std::string load_test_tile();

struct mypoint {
    int64_t p1;
    int64_t p2;

    mypoint() = default;
    mypoint(int64_t x, int64_t y) :
        p1(x),
        p2(y) {
   }
};

inline bool operator==(const mypoint lhs, const mypoint rhs) noexcept {
    return lhs.p1 == rhs.p1 && lhs.p2 == rhs.p2;
}

inline bool operator!=(const mypoint lhs, const mypoint rhs) noexcept {
    return !(lhs == rhs);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const mypoint p) {
    return out << '(' << p.p1 << ',' << p.p2 << ')';
}

inline vtzero::point create_vtzero_point(mypoint p) noexcept {
    return {static_cast<int32_t>(p.p1),
            static_cast<int32_t>(p.p2)};
}

struct mypoint_3d {
    int64_t x = 0;
    int64_t y = 0;
    int64_t elev = 0;

    mypoint_3d(int64_t x_, int64_t y_, int64_t elev_) :
        x(x_),
        y(y_),
        elev(elev_) {
    }
};

inline constexpr bool operator==(const mypoint_3d& a, const mypoint_3d& b) noexcept {
    return a.x == b.x && a.y == b.y && a.elev == b.elev;
}

inline constexpr bool operator!=(const mypoint_3d& a, const mypoint_3d& b) noexcept {
    return !(a == b);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const mypoint_3d& p) {
    return out << '(' << p.x << ',' << p.y << ',' << p.elev << ')';
}

#endif // TEST_HPP
