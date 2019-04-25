
#include <test.hpp>
#include <test_geometry.hpp>
#include <test_geometry_handler.hpp>

#include <cstdint>

using geom_decoder = vtzero::detail::geometry_decoder<2, 0, geom_iterator>;

TEST_CASE("Calling decode_point() with empty input") {
    const geom_container geom;

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "Expected MoveTo command (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with a valid point") {
    const geom_container geom = {command_move_to(1), 50, 34};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    point_handler<2> handler;
    decoder.decode_point(handler);

    const std::vector<vtzero::point_2d> expected = {{25, 17}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Calling decode_point() with a valid multipoint") {
    const geom_container geom = {command_move_to(2), 10, 14, 3, 9};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    point_handler<2> handler;
    decoder.decode_point(handler);

    const std::vector<vtzero::point_2d> expected = {{5, 7}, {3, 2}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Calling decode_point() with a linestring geometry fails") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0}; // this is a linestring geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with a polygon geometry fails") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(2), 10, 12, 24, 44,
                                 command_close_path()}; // this is a polygon geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with something other than MoveTo command") {
    const geom_container geom = {command_line_to(3)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "Expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_point() with a count of 0") {
    const geom_container geom = {command_move_to(0)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "MoveTo command count is zero (spec 4.3.4.2)");
    }
}

TEST_CASE("Calling decode_point() with more data then expected") {
    const geom_container geom = {command_move_to(1), 50, 34, 9};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_point(point_handler<2>{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_point(point_handler<2>{}),
                            "Additional data after end of geometry (spec 4.3.4.2)");
    }
}

