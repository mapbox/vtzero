
#include <test.hpp>
#include <test_geometry.hpp>

#include <cstdint>
#include <vector>

using geom_decoder = vtzero::detail::geometry_decoder<2, 0, geom_iterator>;

class dummy_geom_handler {

    int value = 0;

public:

    void ring_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void ring_point(const vtzero::point_2d /*point*/) noexcept {
        value += 100;
    }

    void ring_end(vtzero::ring_type /*is_outer*/) noexcept {
        value += 10000;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_polygon_geometry() with empty input") {
    const geom_container geom;

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    dummy_geom_handler handler;
    decoder.decode_polygon(dummy_geom_handler{});
    REQUIRE(handler.result() == 0);
}

TEST_CASE("Calling decode_polygon_geometry() with a valid polygon") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(2), 10, 12, 24, 44,
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10401);
}

TEST_CASE("Calling decode_polygon_geometry() with a duplicate end point") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(3), 10, 12, 24, 44, 33, 55,
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10501);
}

TEST_CASE("Calling decode_polygon_geometry() with a valid multipolygon") {
    const geom_container geom = {command_move_to(1), 0, 0,
                                 command_line_to(3), 20, 0, 0, 20, 19, 0,
                                 command_close_path(),
                                 command_move_to(1), 22, 2,
                                 command_line_to(3), 18, 0, 0, 18, 17, 0,
                                 command_close_path(),
                                 command_move_to(1), 4, 13,
                                 command_line_to(3), 0, 8, 8, 0, 0, 7,
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 31503);
}

TEST_CASE("Calling decode_polygon_geometry() with a point geometry fails") {
    const geom_container geom = {command_move_to(1), 50, 34}; // this is a point geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected LineTo command (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a linestring geometry fails") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0}; // this is a linestring geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected ClosePath command (4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with something other than MoveTo command") {
    const geom_container geom = {command_line_to(3)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a count of 0") {
    const geom_container geom = {command_move_to(0)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with a count of 2") {
    const geom_container geom = {command_move_to(2), 1, 2, 3, 4};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.4)");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with 2nd command not a LineTo") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_move_to(1)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_polygon_geometry() with LineTo and 0 count") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_line_to(0),
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10201);
}

TEST_CASE("Calling decode_polygon_geometry() with LineTo and 1 count") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_line_to(1), 5, 6,
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10301);
}

TEST_CASE("Calling decode_polygon_geometry() with 3nd command not a ClosePath") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_line_to(2), 4, 5, 6, 7,
                                 command_line_to(0)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_polygon(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_polygon(dummy_geom_handler{}),
                            "expected command 7 but got 2");
    }
}

TEST_CASE("Calling decode_polygon_geometry() on polygon with zero area") {
    const geom_container geom = {command_move_to(1), 0, 0,
                                 command_line_to(3), 2, 0, 0, 4, 2, 0,
                                 command_close_path()};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    REQUIRE(decoder.decode_polygon(dummy_geom_handler{}) == 10501);
}

