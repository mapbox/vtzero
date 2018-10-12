
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <vector>

using geom_container = std::vector<uint32_t>;
using geom_iterator = geom_container::const_iterator;

using elev_container = std::vector<int64_t>;
using elev_iterator = elev_container::const_iterator;

using attr_container = std::vector<uint64_t>;
using attr_iterator = attr_container::const_iterator;

TEST_CASE("Geometric attributes") {
    const attr_container attr = {10 + (0u << 4u), 2, 0, 9, 7,
                                 10 + (1u << 4u), 2, 0, 7, 4};

    vtzero::detail::geometric_attribute<attr_iterator> ga1{attr.begin() + 3, 0, 0, 2};
    vtzero::detail::geometric_attribute<attr_iterator> ga2{attr.begin() + 8, 0, 0, 2};

    REQUIRE(ga1.get_next_value());
    REQUIRE(ga1.value() == 4);
    REQUIRE(ga2.get_next_value());
    REQUIRE(ga2.value() == 3);
    REQUIRE(ga1.get_next_value());
    REQUIRE(ga1.value() == 7);
    REQUIRE(ga2.get_next_value());
    REQUIRE(ga2.value() == 1);
    REQUIRE_FALSE(ga1.get_next_value());
    REQUIRE_FALSE(ga2.get_next_value());
}

TEST_CASE("Geometric attributes with null values") {
    const attr_container attr = {10 + (0u << 4u), 3, 0, 9, 0, 7,
                                 10 + (1u << 4u), 4, 0, 0, 7, 0, 4};

    vtzero::detail::geometric_attribute<attr_iterator> ga1{attr.begin() + 0 + 3, 0, 0, 3};
    vtzero::detail::geometric_attribute<attr_iterator> ga2{attr.begin() + 6 + 3, 0, 0, 4};

    REQUIRE(ga1.get_next_value());
    REQUIRE(ga1.value() == 4);
    REQUIRE_FALSE(ga1.get_next_value());
    REQUIRE(ga1.get_next_value());
    REQUIRE(ga1.value() == 7);
    REQUIRE_FALSE(ga1.get_next_value());

    REQUIRE_FALSE(ga2.get_next_value());
    REQUIRE(ga2.get_next_value());
    REQUIRE(ga2.value() == 3);
    REQUIRE_FALSE(ga2.get_next_value());
    REQUIRE(ga2.get_next_value());
    REQUIRE(ga2.value() == 1);
    REQUIRE_FALSE(ga2.get_next_value());
}

struct point_with_attr {
    int64_t x = 0;
    int64_t y = 0;
    int64_t elev = 0;
    int64_t attr1 = 0;
    int64_t attr2 = 0;

    point_with_attr(int64_t x_, int64_t y_, int64_t elev_, int64_t a1 = 0, int64_t a2 = 0) :
        x(x_),
        y(y_),
        elev(elev_),
        attr1(a1),
        attr2(a2) {
    }
};

inline constexpr bool operator==(const point_with_attr& a, const point_with_attr& b) noexcept {
    return a.x == b.x &&
           a.y == b.y &&
           a.elev == b.elev &&
           a.attr1 == b.attr1 &&
           a.attr2 == b.attr2;
}

inline constexpr bool operator!=(const point_with_attr& a, const point_with_attr& b) noexcept {
    return !(a == b);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const point_with_attr& p) {
    return out << '(' << p.x << ',' << p.y << ',' << p.elev << ',' << p.attr1 << ',' << p.attr2 << ')';
}

class geom_with_attr_handler {

    std::vector<point_with_attr> m_points;

public:

    static point_with_attr convert(const vtzero::unscaled_point& p) noexcept {
        return {p.x, p.y, p.z};
    }

    void points_begin(const uint32_t /*count*/) const noexcept {
    }

    void points_point(const point_with_attr& point) noexcept {
        m_points.push_back(point);
    }

    void points_end() const noexcept {
    }

    void linestring_begin(const uint32_t /*count*/) const noexcept {
    }

    void linestring_point(const point_with_attr& point) noexcept {
        m_points.push_back(point);
    }

    void linestring_end() const noexcept {
    }

    void points_attr(vtzero::index_value key_index, vtzero::index_value /*scaling_index*/, int64_t value) noexcept {
        assert(!m_points.empty());
        auto& p = m_points.back();
        if (key_index.value() == 0) {
            p.attr1 = value;
        } else {
            p.attr2 = value;
        }
    }

    void points_null_attr(vtzero::index_value /*key_index*/) noexcept {
    }

    const std::vector<point_with_attr>& result() const noexcept {
        return m_points;
    }

}; // class geom_with_attr_handler

TEST_CASE("Calling decode_point() decoding valid multipoint with geometric attributes") {
    const geom_container geom = {17, 10, 14, 3, 9};
    const elev_container elev = {22, 3};
    const attr_container attr = {10 + (0u << 4u), 2, 0, 9, 7,
                                 10 + (1u << 4u), 2, 0, 7, 4};

    vtzero::detail::extended_geometry_decoder<3, 4, geom_iterator, elev_iterator, attr_iterator> decoder{
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend(),
        attr.cbegin(), attr.cend(),
        geom.size() / 2};

    geom_with_attr_handler handler;
    const auto result = decoder.decode_point(handler);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == point_with_attr(5, 7, 22, 4, 3));
    REQUIRE(result[1] == point_with_attr(3, 2, 25, 7, 1));
}

TEST_CASE("Calling decode_linestring() decoding valid linestring with geometric attributes") {
    const geom_container geom = {9, 4, 4, 18, 0, 16, 16, 0};
    const elev_container elev = {22, 3, 4};
    const attr_container attr = {10 + (0u << 4u), 2, 0, 9, 7,
                                 10 + (1u << 4u), 2, 0, 7, 4};

    vtzero::detail::extended_geometry_decoder<3, 4, geom_iterator, elev_iterator, attr_iterator> decoder{
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend(),
        attr.cbegin(), attr.cend(),
        geom.size() / 2};

    geom_with_attr_handler handler;
    const auto result = decoder.decode_linestring(handler);

    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == point_with_attr(2, 2, 22, 4, 3));
    REQUIRE(result[1] == point_with_attr(2, 10, 25, 7, 1));
    REQUIRE(result[2] == point_with_attr(10, 10, 29, 0, 0));
}

