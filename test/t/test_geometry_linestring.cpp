
#include <test.hpp>

#include <vtzero/geometry.hpp>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

struct dummy_geom_handler {

    int value = 0;

    void linestring_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void linestring_point(const vtzero::point /*point*/) noexcept {
        value += 100;
    }

    void linestring_end() noexcept {
        value += 10000;
    }

}; // dummy_geom_handler

TEST_CASE("Calling decode_linestring_geometry() with empty input") {
    const container g;
    dummy_geom_handler handler;
    vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{});
    REQUIRE(handler.value == 0);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid linestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0};
    dummy_geom_handler handler;
    vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, handler);
    REQUIRE(handler.value == 10301);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid multilinestring") {
    const container g = {9, 4, 4, 18, 0, 16, 16, 0, 9, 17, 17, 10, 4, 8};
    dummy_geom_handler handler;
    vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, handler);
    REQUIRE(handler.value == 20502);
}

TEST_CASE("Calling decode_linestring_geometry() with a point geometry fails") {
    const container g = {9, 50, 34}; // this is a point geometry
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected LineTo command (spec 4.3.4.3)");
}

TEST_CASE("Calling decode_linestring_geometry() with a polygon geometry fails") {
    const container g = {9, 6, 12, 18, 10, 12, 24, 44, 15}; // this is a polygon geometry
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected command 1 but got 7");
}

TEST_CASE("Calling decode_linestring_geometry() with something other than MoveTo command") {
    const container g = {vtzero::detail::command_line_to(3)};
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected command 1 but got 2");
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 0") {
    const container g = {vtzero::detail::command_move_to(0)};
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "MoveTo command count is not 1 (spec 4.3.4.3)");
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 2") {
    const container g = {vtzero::detail::command_move_to(2)};
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "MoveTo command count is not 1 (spec 4.3.4.3)");
}

TEST_CASE("Calling decode_linestring_geometry() with 2nd command not a LineTo") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4, vtzero::detail::command_move_to(1)};
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "expected command 2 but got 1");
}

TEST_CASE("Calling decode_linestring_geometry() with LineTo and 0 count") {
    const container g = {vtzero::detail::command_move_to(1), 3, 4, vtzero::detail::command_line_to(0)};
    REQUIRE_THROWS_AS(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                      const vtzero::geometry_exception&);
    REQUIRE_THROWS_WITH(vtzero::decode_linestring_geometry(g.cbegin(), g.cend(), true, dummy_geom_handler{}),
                        "LineTo command count is zero (spec 4.3.4.3)");
}

