
#include <test.hpp>

#include <vtzero/geometry.hpp>

TEST_CASE("default constructed point") {
    vtzero::point_2d p{};

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);
}

TEST_CASE("point") {
    vtzero::point_2d p1{4, 5};
    vtzero::point_2d p2{5, 4};
    vtzero::point_2d p3{4, 5};

    REQUIRE(p1.x == 4);
    REQUIRE(p1.y == 5);

    REQUIRE_FALSE(p1 == p2);
    REQUIRE(p1 != p2);
    REQUIRE(p1 == p3);
}

