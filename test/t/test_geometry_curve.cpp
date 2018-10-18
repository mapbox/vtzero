
#include <test.hpp>

#include <vtzero/geometry.hpp>

#include <cstdint>
#include <vector>

using geom_container = std::vector<uint32_t>;
using geom_iterator = geom_container::const_iterator;

using elev_iterator = vtzero::detail::dummy_elev_iterator;

using knot_container = std::vector<double>;
using knot_iterator = knot_container::const_iterator;

using attr_iterator = vtzero::detail::dummy_attr_iterator;

class dummy_geom_handler {

    int value = 0;

public:

    static vtzero::point_2d convert(const vtzero::point_2d p) noexcept {
        return p;
    }

    void controlpoints_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void controlpoints_point(const vtzero::point_2d /*point*/) noexcept {
        value += 100;
    }

    void controlpoints_end() noexcept {
        value += 10000;
    }

    void knots_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void knots_value(double /*value*/) noexcept {
        value += 2;
    }

    void knots_end() noexcept {
        value += 200;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_curve_geometry() with empty input") {
    const geom_container geom;
    const knot_container knot;

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    dummy_geom_handler handler;
    decoder.decode_curve(dummy_geom_handler{});
    REQUIRE(handler.result() == 0);
}

TEST_CASE("Calling decode_curve_geometry() with a valid curve") {
    const geom_container geom = {9, 4, 4, 18, 0, 16, 16, 0};
    const knot_container knot = {0.0, 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    REQUIRE(decoder.decode_curve(dummy_geom_handler{}) == 10522);
}

TEST_CASE("Calling decode_curve_geometry() with a point geometry fails") {
    const geom_container geom = {9, 50, 34}; // this is a point geometry
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "expected LineTo command (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_curve_geometry() with a polygon geometry fails") {
    const geom_container geom = {9, 6, 12, 18, 10, 12, 24, 44, 15}; // this is a polygon geometry
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_curve_geometry() with something other than MoveTo command") {
    const geom_container geom = {vtzero::detail::command_line_to(3)};
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_curve_geometry() with a count of 0") {
    const geom_container geom = {vtzero::detail::command_move_to(0)};
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_curve_geometry() with a count of 2") {
    const geom_container geom = {vtzero::detail::command_move_to(2), 10, 20, 20, 10};
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_curve_geometry() with 2nd command not a LineTo") {
    const geom_container geom = {vtzero::detail::command_move_to(1), 3, 4,
                                 vtzero::detail::command_move_to(1)};
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_curve_geometry() with LineTo and 0 count") {
    const geom_container geom = {vtzero::detail::command_move_to(1), 3, 4,
                                 vtzero::detail::command_line_to(0)};
    const knot_container knot = {1.0, 1.0, 1.0, 1.0};

    vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator, knot_iterator, attr_iterator> decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev_iterator{}, elev_iterator{},
        knot.cbegin(), knot.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_curve(dummy_geom_handler{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_curve(dummy_geom_handler{}),
                            "LineTo command count is zero (spec 4.3.4.3)");
    }
}

