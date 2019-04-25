
#include <test.hpp>
#include <test_geometry.hpp>
#include <test_geometry_handler.hpp>
#include <test_point.hpp>

#include <vtzero/detail/geometry.hpp>

#include <cstdint>
#include <vector>

using geom_container = std::vector<uint32_t>;
using geom_iterator = geom_container::const_iterator;

using elev_container = std::vector<int32_t>;
using elev_iterator = elev_container::const_iterator;

using geom_decoder = vtzero::detail::geometry_decoder<3, 0, geom_iterator, elev_iterator>;

TEST_CASE("Extended geometry: Calling decode_point() with empty input") {
    const geom_container geom = {};
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "Expected MoveTo command (spec 4.3.4.2)");
    }
}

TEST_CASE("Extended geometry: Calling decode_point() with a valid point") {
    const geom_container geom = {command_move_to(1), 50, 34};
    const elev_container elev = {15};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    point_handler<3> handler;
    decoder.decode_point(handler);

    std::vector<vtzero::point_3d> expected = {{25, 17, 15}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Extended geometry: Calling decode_point() with a valid multipoint") {
    const geom_container geom = {command_move_to(2), 10, 14, 3, 9};
    const elev_container elev = {22, 3};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    point_handler<3> handler;
    decoder.decode_point(handler);

    std::vector<vtzero::point_3d> expected = {{5, 7, 22}, {3, 2, 25}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Extended geometry: Calling decode_point() with a linestring geometry fails") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0}; // this is a linestring geometry
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Extended geometry: Calling decode_point() with a polygon geometry fails") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(2), 10, 12, 24, 44,
                                 command_close_path()}; // this is a polygon geometry
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Extended geometry: Calling decode_point() with something other than MoveTo command") {
    const geom_container geom = {command_line_to(3)};
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "Expected command 1 but got 2");
    }
}

TEST_CASE("Extended geometry: Calling decode_point() with a count of 0") {
    const geom_container geom = {command_move_to(0)};
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "MoveTo command count is zero (spec 4.3.4.2)");
    }
}

TEST_CASE("Extended geometry: Calling decode_point() with more data then expected") {
    const geom_container geom = {command_move_to(1), 50, 34, 9};
    const elev_container elev = {};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<3>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<3>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

class value_geom_handler {

    std::vector<test_point_3d> m_points;

public:

    constexpr static const int dimensions = 3;
    constexpr static const unsigned int max_geometric_attributes = 0;

    static test_point_3d convert(const vtzero::point_3d& p) noexcept {
        return {p.x, p.y, p.z};
    }

    void points_begin(const uint32_t /*count*/) const noexcept {
    }

    void points_point(const test_point_3d& point) noexcept {
        m_points.push_back(point);
    }

    void points_end() const noexcept {
    }

    const std::vector<test_point_3d>& result() const noexcept {
        return m_points;
    }

}; // class value_geom_handler

TEST_CASE("Extended geometry: Calling decode_point() decoding valid multipoint") {
    const geom_container geom = {command_move_to(2), 10, 14, 3, 9};
    const elev_container elev = {22, 3};

    geom_decoder decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    value_geom_handler handler;
    const auto result = decoder.decode_point(handler);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == test_point_3d(5, 7, 22));
    REQUIRE(result[1] == test_point_3d(3, 2, 25));
}

