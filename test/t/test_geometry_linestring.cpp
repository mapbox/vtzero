
#include <test.hpp>
#include <test_geometry.hpp>

#include <cstdint>
#include <vector>

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;
using geom_decoder = vtzero::detail::geometry_decoder<2, 0, iterator>;

class dummy_geom_handler {

    int value = 0;

public:

    static vtzero::point_2d convert(const vtzero::point_2d p) noexcept {
        return p;
    }

    void linestring_begin(const uint32_t /*count*/) noexcept {
        ++value;
    }

    void linestring_point(const vtzero::point_2d /*point*/) noexcept {
        value += 100;
    }

    void linestring_end() noexcept {
        value += 10000;
    }

    int result() const noexcept {
        return value;
    }

}; // class dummy_geom_handler

TEST_CASE("Calling decode_linestring_geometry() with empty input") {
    const container g;
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    dummy_geom_handler handler;
    decoder.decode_linestring(dummy_geom_handler{});
    REQUIRE(handler.result() == 0);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid linestring") {
    const container g = {command_move_to(1), 4, 4,
                         command_line_to(2), 0, 16, 16, 0};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    REQUIRE(decoder.decode_linestring(dummy_geom_handler{}) == 10301);
}

TEST_CASE("Calling decode_linestring_geometry() with a valid multilinestring") {
    const container g = {command_move_to(1), 4, 4,
                         command_line_to(2), 0, 16, 16, 0,
                         command_move_to(1), 17, 17,
                         command_line_to(1), 4, 8};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    dummy_geom_handler handler;
    decoder.decode_linestring(handler);
    REQUIRE(handler.result() == 20502);
}

TEST_CASE("Calling decode_linestring_geometry() with a point geometry fails") {
    const container g = {command_move_to(1), 50, 34}; // this is a point geometry
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected LineTo command (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a polygon geometry fails") {
    const container g = {command_move_to(1), 6, 12,
                         command_line_to(2), 10, 12, 24, 44,
                         command_close_path()}; // this is a polygon geometry
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 1 but got 7");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with something other than MoveTo command") {
    const container g = {command_line_to(3)};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 1 but got 2");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 0") {
    const container g = {command_move_to(0)};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with a count of 2") {
    const container g = {command_move_to(2), 10, 20, 20, 10};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "MoveTo command count is not 1 (spec 4.3.4.3)");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with 2nd command not a LineTo") {
    const container g = {command_move_to(1), 3, 4,
                         command_move_to(1)};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "expected command 2 but got 1");
    }
}

TEST_CASE("Calling decode_linestring_geometry() with LineTo and 0 count") {
    const container g = {command_move_to(1), 3, 4,
                         command_line_to(0)};
    geom_decoder decoder{g.size() / 2, g.cbegin(), g.cend()};

    SECTION("check exception type") {
        REQUIRE_THROWS_AS(decoder.decode_linestring(dummy_geom_handler{}),
                          const vtzero::geometry_exception&);
    }
    SECTION("check exception message") {
        REQUIRE_THROWS_WITH(decoder.decode_linestring(dummy_geom_handler{}),
                            "LineTo command count is zero (spec 4.3.4.3)");
    }
}

