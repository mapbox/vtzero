
#include <test.hpp>

#include <vtzero/geometry.hpp>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

struct dummy_geom_handler {

    int value = 0;

    void points_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void points_point(const vtzero::point /*point*/) noexcept {
        value += 100;
    }

    void points_end() noexcept {
        value += 10000;
    }

}; // dummy_geom_handler

TEST_CASE("Calling decode_point_geometry() with empty input") {
    const container g;

    dummy_geom_handler handler;
    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected MoveTo command (spec 4.3.4.2)");
}

TEST_CASE("Calling decode_point_geometry() with a valid point") {
    const container g = {9, 50, 34};

    dummy_geom_handler handler;
    vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, handler);
    REQUIRE(handler.value == 10101);
}

TEST_CASE("Calling decode_point_geometry() with a valid multipoint") {
    const container g = {17, 10, 14, 3, 9};

    dummy_geom_handler handler;
    vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, handler);
    REQUIRE(handler.value == 10201);
}

TEST_CASE("Calling decode_point_geometry() with a linestring geometry fails") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0}; // this is a linestring geometry

    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "additional data after end of geometry (spec 4.3.4.2)");
}

TEST_CASE("Calling decode_point_geometry() with a polygon geometry fails") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15}; // this is a polygon geometry

    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "additional data after end of geometry (spec 4.3.4.2)");
}

TEST_CASE("Calling decode_point_geometry() with something other than MoveTo command") {
    const container g = {vtzero::detail::command_line_to(3)};

    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected command 1 but got 2");
}

TEST_CASE("Calling decode_point_geometry() with a count of 0") {
    const container g = {vtzero::detail::command_move_to(0)};

    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "MoveTo command count is zero (spec 4.3.4.2)");
}

TEST_CASE("Calling decode_point_geometry() with more data then expected") {
    const container g = {9, 50, 34, 9};

    REQUIRE_THROWS_AS(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_point_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "additional data after end of geometry (spec 4.3.4.2)");
}

