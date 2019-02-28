
#include <test.hpp>

#include <vtzero/point.hpp>

#include <type_traits>

static_assert(std::is_nothrow_move_constructible<vtzero::point_2d>::value, "point_2d is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<vtzero::point_2d>::value, "point_2d is not nothrow move assignable");

static_assert(std::is_nothrow_move_constructible<vtzero::point_3d>::value, "point_3d is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<vtzero::point_3d>::value, "point_3d is not nothrow move assignable");

TEST_CASE("default constructed 2d point") {
    vtzero::point_2d p{};

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);
}

TEST_CASE("default constructed 3d point") {
    vtzero::point_3d p{};

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);
    REQUIRE(p.z == 0);
}

TEST_CASE("2d point") {
    vtzero::point_2d p1{4, 5};
    vtzero::point_2d p2{5, 4};
    vtzero::point_2d p3{4, 5};

    REQUIRE(p1.x == 4);
    REQUIRE(p1.y == 5);

    REQUIRE_FALSE(p1 == p2);
    REQUIRE(p1 != p2);
    REQUIRE(p1 == p3);
}

TEST_CASE("2d point z coordinate") {
    vtzero::point_2d p{1, 2};
    REQUIRE(p.get_z() == 0);
    p.set_z(17);
    REQUIRE(p.get_z() == 0);
    p.add_to_z(22);
    REQUIRE(p.get_z() == 0);
}

TEST_CASE("3d point") {
    vtzero::point_3d p1{4, 5, 1};
    vtzero::point_3d p2{5, 4, 3};
    vtzero::point_3d p3{4, 5, 1};
    vtzero::point_3d p4{4, 5, 3};

    REQUIRE(p1.x == 4);
    REQUIRE(p1.y == 5);
    REQUIRE(p1.z == 1);

    REQUIRE_FALSE(p1 == p2);
    REQUIRE(p1 != p2);
    REQUIRE(p1 == p3);
    REQUIRE(p1 != p4);
}

TEST_CASE("3d point z coordinate") {
    vtzero::point_3d p{1, 2};
    REQUIRE(p.get_z() == 0);
    p.set_z(17);
    REQUIRE(p.get_z() == 17);
    p.add_to_z(22);
    REQUIRE(p.get_z() == 39);
}

