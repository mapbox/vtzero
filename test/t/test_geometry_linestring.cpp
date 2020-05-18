
#include <test.hpp>
#include <test_geometry.hpp>
#include <test_geometry_handler.hpp>

#include <cstdint>

using geom_decoder = vtzero::detail::geometry_decoder<2, 0, geom_iterator>;

TEST_CASE("Calling decode_linestring_geometry() with empty input") {
    const geom_container geom;

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    linestring_handler<2> handler;
    decoder.decode_linestring(handler);

    const std::vector<std::vector<vtzero::point_2d>> expected = {};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid linestring") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    linestring_handler<2> handler;
    decoder.decode_linestring(handler);

    const std::vector<std::vector<vtzero::point_2d>> expected = {{{2, 2}, {2, 10}, {10, 10}}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid multilinestring") {
    const geom_container geom = {command_move_to(1), 4, 4,
                                 command_line_to(2), 0, 16, 16, 0,
                                 command_move_to(1), 17, 17,
                                 command_line_to(1), 4, 8};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    linestring_handler<2> handler;
    decoder.decode_linestring(handler);

    const std::vector<std::vector<vtzero::point_2d>> expected = {{{2, 2}, {2, 10}, {10, 10}},
                                                                 {{1, 1}, {3, 5}}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Calling decode_linestring_geometry() with a point geometry fails") {
    const geom_container geom = {command_move_to(1), 50, 34}; // this is a point geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "Expected LineTo command (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a polygon geometry fails") {
    const geom_container geom = {command_move_to(1), 6, 12,
                                 command_line_to(2), 10, 12, 24, 44,
                                 command_close_path()}; // this is a polygon geometry

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "Expected command 1 but got 7");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with something other than MoveTo command") {
    const geom_container geom = {command_line_to(3)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "Expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 0") {
    const geom_container geom = {command_move_to(0)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 2") {
    const geom_container geom = {command_move_to(2), 10, 20, 20, 10};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with 2nd command not a LineTo") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_move_to(1)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "Expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with LineTo and 0 count") {
    const geom_container geom = {command_move_to(1), 3, 4,
                                 command_line_to(0)};

    geom_decoder decoder{geom.size() / 2, geom.cbegin(), geom.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(linestring_handler<2>{}),
                          vtzero::geometry_exception);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(linestring_handler<2>{}),
                            "LineTo command count is zero (spec 4.3.4.3)");
    }
}

