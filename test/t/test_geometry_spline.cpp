
#include <test.hpp>
#include <test_attr.hpp>
#include <test_geometry.hpp>
#include <test_geometry_handler.hpp>

#include <cstdint>
#include <vector>

using elev_iterator = vtzero::detail::dummy_elev_iterator;

using knot_container = std::vector<uint64_t>;
using knot_iterator = knot_container::const_iterator;

using attr_iterator = vtzero::detail::dummy_attr_iterator;

TEST_CASE("Calling decode_spline_geometry() with empty input") {
    const geom_container geom;
    const knot_container knot;

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    spline_handler<2> handler;
    decoder.decode_spline(handler, 2);

    const std::vector<std::vector<vtzero::point_2d>> expected_control_points = {};
    const std::vector<int64_t> expected_knots = {};
    REQUIRE(handler.control_points == expected_control_points);
    REQUIRE(handler.knots == expected_knots);
}

TEST_CASE("Calling decode_spline_geometry() with a valid spline") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0};
    const knot_container knot = {number_list(6), 0, 1, 1, 1, 1, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    spline_handler<2> handler;
    decoder.decode_spline(handler, 2);

    const std::vector<std::vector<vtzero::point_2d>> expected_control_points = {{{2, 2}, {2, 10}, {10, 10}}};
    const std::vector<int64_t> expected_knots = {0, 0, 0, 0, 0, 0};
    REQUIRE(handler.control_points == expected_control_points);
    REQUIRE(handler.knots == expected_knots);
}

TEST_CASE("Calling decode_spline_geometry() with a point geometry fails") {
    const geom_container geom = {command_move_to(1), 50, 34}; // this is a point geometry
    const knot_container knot = {number_list(4), 0, 1, 0, 0, 0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "Expected LineTo command (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_spline_geometry() with a polygon geometry fails") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(2), 10, 12, 24, 44,
                                 command_close_path()}; // this is a polygon geometry
    const knot_container knot = {number_list(6), 0, 1, 1, 1, 1, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "Expected command 1 but got 7");
    }
}

TEST_CASE("Calling decode_spline_geometry() with something other than MoveTo command") {
    const geom_container geom = {command_line_to(3)};
    const knot_container knot = {number_list(2), 0, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "Expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_spline_geometry() with a count of 0") {
    const geom_container geom = {command_move_to(0)};
    const knot_container knot = {number_list(2), 0, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_spline_geometry() with a count of 2") {
    const geom_container geom = {command_move_to(2), 10, 20, 20, 10};
    const knot_container knot = {number_list(2), 0, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_spline_geometry() with 2nd command not a LineTo") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_move_to(1)};
    const knot_container knot = {number_list(3), 0, 1, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "Expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_spline_geometry() with LineTo and 0 count") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_line_to(0)};
    const knot_container knot = {number_list(3), 0, 1, 1, 1};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_spline(spline_handler<2>{}, 2),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_spline(spline_handler<2>{}, 2),
                            "LineTo command count is zero (spec 4.3.4.3)");
    }
}

