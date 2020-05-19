
#include <test.hpp>
#include <test_point.hpp>

#include <vtzero/detail/geometry.hpp>

#include <cstdint>
#include <limits>
#include <vector>

using geom_container = std::vector<uint32_t>;
using geom_iterator = geom_container::const_iterator;

using elev_container = std::vector<int32_t>;
using elev_iterator = elev_container::const_iterator;

using geom_decoder3 = vtzero::detail::geometry_decoder<3, 0, geom_iterator, elev_iterator>;
using geom_decoder2 = vtzero::detail::geometry_decoder<2, 0, geom_iterator, elev_iterator>;

TEST_CASE("3d geometry_decoder") {
    const geom_container geom = {};
    const elev_container elev = {};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE_THROWS_AS(decoder.next_point(), assert_error);
}

TEST_CASE("3d geometry_decoder with point") {
    const geom_container geom = {9, 50, 34};
    const elev_container elev = {12};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_point(), assert_error);

    SECTION("trying to get LineTo command") {
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::LINE_TO), vtzero::geometry_exception);
    }

    SECTION("trying to get ClosePath command") {
        REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH),
                            "Expected command 7 but got 1");
    }

    SECTION("trying to get MoveTo command") {
        REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
        REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), assert_error);
        REQUIRE(decoder.count() == 1);
        REQUIRE(decoder.next_point() == vtzero::point_3d(25, 17, 12));

        REQUIRE(decoder.done());
        REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    }
}

TEST_CASE("3d geometry_decoder with point and 2d coordinates") {
    const geom_container geom = {9, 50, 34};
    const elev_container elev = {};

    geom_decoder2 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_point(), assert_error);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_2d(25, 17));

    REQUIRE(decoder.done());
}

TEST_CASE("3d geometry_decoder with incomplete point") {
    geom_container geom = {9, 50, 34};

    SECTION("half a point") {
        geom.pop_back();
    }

    SECTION("missing point") {
        geom.pop_back();
        geom.pop_back();
    }

    const elev_container elev = {};

    geom_decoder3 decoder{
        100,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE_THROWS_AS(decoder.next_point(), vtzero::geometry_exception);
}

TEST_CASE("3d geometry_decoder with multipoint") {
    const geom_container geom = {17, 10, 14, 3, 9};
    const elev_container elev = {12, 4};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point_3d(5, 7, 12));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 2, 16));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("3d geometry_decoder with multipoint and missing elevation field") {
    const geom_container geom = {17, 10, 14, 3, 9};
    const elev_container elev = {12};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(5, 7, 12));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 2, 12));
    REQUIRE(decoder.count() == 0);
}

TEST_CASE("3d geometry_decoder with multipoint and additional elevation field") {
    const geom_container geom = {17, 10, 14, 3, 9};
    const elev_container elev = {3, 4, 7};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(5, 7, 3));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 2, 7));
    REQUIRE(decoder.count() == 0);

    REQUIRE_FALSE(decoder.done());
}

TEST_CASE("3d geometry_decoder with linestring") {
    const geom_container geom = {9, 4, 4, 18, 0, 16, 16, 0};
    const elev_container elev = {1, 10, 3};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 2, 1));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 10, 11));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(10, 10, 14));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("3d geometry_decoder with linestring with equal points") {
    const geom_container geom = {9, 4, 4, 18, 0, 16, 0, 0};
    const elev_container elev = {6, -8, 2};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 2, 6));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 10, -2));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 10, 0));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
}

TEST_CASE("3d geometry_decoder with multilinestring") {
    const geom_container geom = {9, 4, 4, 18, 0, 16, 16, 0, 9, 17, 17, 10, 4, 8};
    const elev_container elev = {1, 2, 1, 4, 1};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 2, 1));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(2, 10, 3));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(10, 10, 4));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(1, 1, 8));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 5, 9));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("3d geometry_decoder with polygon") {
    const geom_container geom = {9, 6, 12, 18, 10, 12, 24, 44, 15};
    const elev_container elev = {-3, 20, 6};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 6, -3));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 2);

    REQUIRE(decoder.next_point() == vtzero::point_3d(8, 12, 17));
    REQUIRE(decoder.count() == 1);

    REQUIRE(decoder.next_point() == vtzero::point_3d(20, 34, 23));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("3d geometry_decoder with polygon with wrong ClosePath count 2") {
    const geom_container geom = {9, 6, 12, 18, 10, 12, 24, 44, 23};
    const elev_container elev = {1, -1, 1};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));

    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 6, 1));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.next_point() == vtzero::point_3d(8, 12, 0));
    REQUIRE(decoder.next_point() == vtzero::point_3d(20, 34, 1));
    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), vtzero::geometry_exception);
    REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH),
                        "ClosePath command count is not 1");
}

TEST_CASE("3d geometry_decoder with polygon with wrong ClosePath count 0") {
    const geom_container geom = {9, 6, 12, 18, 10, 12, 24, 44, 7};
    const elev_container elev = {0, 0, 0};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.next_point() == vtzero::point_3d(3, 6));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.next_point() == vtzero::point_3d(8, 12));
    REQUIRE(decoder.next_point() == vtzero::point_3d(20, 34));
    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH), vtzero::geometry_exception);
    REQUIRE_THROWS_WITH(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH),
                        "ClosePath command count is not 1");
}

TEST_CASE("3d geometry_decoder with multipolygon") {
    const geom_container geom = {9, 0, 0, 26, 20, 0, 0, 20, 19, 0, 15, 9, 22, 2, 26, 18,
                                 0, 0, 18, 17, 0, 15, 9, 4, 13, 26, 0, 8, 8, 0, 0, 7, 15};
    const elev_container elev = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(0, 0, 0));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point_3d(10, 0, 1));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point_3d(10, 10, 2));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(0, 10, 3));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(11, 11, 4));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point_3d(20, 11, 5));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point_3d(20, 20, 6));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(11, 20, 7));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(13, 13, 8));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 3);
    REQUIRE(decoder.next_point() == vtzero::point_3d(13, 17, 9));
    REQUIRE(decoder.count() == 2);
    REQUIRE(decoder.next_point() == vtzero::point_3d(17, 17, 10));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_3d(17, 13, 11));
    REQUIRE(decoder.count() == 0);
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::CLOSE_PATH));
    REQUIRE(decoder.count() == 0);

    REQUIRE(decoder.done());
    REQUIRE_FALSE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
}

TEST_CASE("3d geometry_decoder decoding linestring with int32 overflow in x coordinate") {
    const geom_container geom = {vtzero::detail::command_move_to(1),
                                   protozero::encode_zigzag32(std::numeric_limits<int32_t>::max()),
                                   protozero::encode_zigzag32(0),
                                 vtzero::detail::command_line_to(1),
                                   protozero::encode_zigzag32(1),
                                   protozero::encode_zigzag32(1)
                                };
    const elev_container elev = {};

    geom_decoder2 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_2d(std::numeric_limits<int32_t>::max(), 0));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_2d(std::numeric_limits<int32_t>::min(), 1));
}

TEST_CASE("3d geometry_decoder decoding linestring with int32 overflow in y coordinate") {
    const geom_container geom = {vtzero::detail::command_move_to(1),
                                   protozero::encode_zigzag32(0),
                                   protozero::encode_zigzag32(std::numeric_limits<int32_t>::min()),
                                 vtzero::detail::command_line_to(1),
                                   protozero::encode_zigzag32(-1),
                                   protozero::encode_zigzag32(-1)
                                };
    const elev_container elev = {};

    geom_decoder2 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE(decoder.next_command(vtzero::detail::CommandId::MOVE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_2d(0, std::numeric_limits<int32_t>::min()));
    REQUIRE(decoder.next_command(vtzero::detail::CommandId::LINE_TO));
    REQUIRE(decoder.count() == 1);
    REQUIRE(decoder.next_point() == vtzero::point_2d(-1, std::numeric_limits<int32_t>::max()));
}

TEST_CASE("3d geometry_decoder with multipoint with a huge count") {
    const uint32_t huge_value = (1UL << 29U) - 1;
    const geom_container geom = {vtzero::detail::command_move_to(huge_value), 10, 10};
    const elev_container elev = {};

    geom_decoder3 decoder{
        geom.size() / 2,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), vtzero::geometry_exception);
}

TEST_CASE("3d geometry_decoder with multipoint with not enough points") {
    const geom_container geom = {vtzero::detail::command_move_to(2), 10};
    const elev_container elev = {};

    geom_decoder3 decoder{
        1,
        geom.cbegin(), geom.cend(),
        elev.cbegin(), elev.cend()};

    REQUIRE(decoder.count() == 0);
    REQUIRE_FALSE(decoder.done());

    REQUIRE_THROWS_AS(decoder.next_command(vtzero::detail::CommandId::MOVE_TO), vtzero::geometry_exception);
}

