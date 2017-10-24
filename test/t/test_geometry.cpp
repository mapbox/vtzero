
#include <test.hpp>

#include <vtzero/geometry.hpp>

TEST_CASE("default constructed point") {
    vtzero::point p{};

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);
}

TEST_CASE("point") {
    vtzero::point p1{4, 5};
    vtzero::point p2{5, 4};
    vtzero::point p3{4, 5};

    REQUIRE(p1.x == 4);
    REQUIRE(p1.y == 5);

    REQUIRE_FALSE(p1 == p2);
    REQUIRE(p1 != p2);
    REQUIRE(p1 == p3);
}

TEST_CASE("geometry_decoder") {
    vtzero::detail::geometry_decoder decoder{vtzero::data_view{}};

    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE_THROWS_AS(decoder.next_point(), const assert_error&);
}

using container = std::vector<uint32_t>;
using iterator = container::const_iterator;

TEST_CASE("geometry_decoder with point") {
    container g = {9, 50, 34};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_point(), const assert_error&);

    SECTION("trying to get LineTo command") {
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::command_line_to()), const vtzero::geometry_exception&);
    }

    SECTION("trying to get ClosePath command") {
        REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::command_close_path()), "expected command 7 but got 1");
    }

    SECTION("trying to get MoveTo command") {
        REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::command_move_to()), const assert_error&);
        REQUIRE(decoder.count() == 1);
        REQUIRE(decoder.next_point() == vtzero::point(25, 17));

        REQUIRE(decoder.done());
        REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
    }
}

TEST_CASE("geometry_decoder with incomplete point") {
    container g = {9, 50, 34};

    SECTION("half a point") {
        g.pop_back();
    }

    SECTION("missing point") {
        g.pop_back();
        g.pop_back();
    }

    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE_THROWS_AS(decoder.next_point(), const vtzero::geometry_exception&);
}

TEST_CASE("geometry_decoder with multipoint") {
    container g = {17, 10, 14, 3, 9};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(5, 7));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 2));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
}

TEST_CASE("geometry_decoder with linestring") {
    container g = {9, 4, 4, 18, 0, 16, 16, 0};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
}

TEST_CASE("geometry_decoder with linestring with equal points") {
    bool strict = false;

    SECTION("with strict") {
        strict = true;
    }

    SECTION("without strict") {
        strict = false;
    }

    container g = {9, 4, 4, 18, 0, 16, 0, 0};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend(), strict};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);

    if (strict) {
        REQUIRE_THROWS_AS(decoder.next_point(), const vtzero::geometry_exception&);
    } else {
        REQUIRE(decoder.next_point() == vtzero::point(2, 10));
        REQUIRE(decoder.count() == 0);

        REQUIRE(decoder.done());
    }
}

TEST_CASE("geometry_decoder with multilinestring") {
    container g = {9, 4, 4, 18, 0, 16, 16, 0, 9, 17, 17, 10, 4, 8};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(2, 2));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(2, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(1, 1));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 5));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
}

TEST_CASE("geometry_decoder with polygon") {
    container g = {9, 6, 12, 18, 10, 12, 24, 44, 15};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(3, 6));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(8, 12));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(20, 34));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::command_close_path()));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
}

TEST_CASE("geometry_decoder with multipolygon") {
    container g = {9, 0, 0, 26, 20, 0, 0, 20, 19, 0, 15, 9, 22, 2, 26, 18,
                   0, 0, 18, 17, 0, 15, 9, 4, 13, 26, 0, 8, 8, 0, 0, 7, 15};
    vtzero::detail::basic_geometry_decoder<iterator> decoder{g.cbegin(), g.cend()};
    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(0, 0));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(10, 0));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(10, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(0, 10));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::command_close_path()));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(11, 11));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(20, 11));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(20, 20));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(11, 20));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::command_close_path()));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::command_move_to()));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(13, 13));
    REQUIRE(decoder.next_command(vtzero::detail::command_line_to()));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point(13, 17));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point(17, 17));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point(17, 13));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::command_close_path()));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::command_move_to()));
}

