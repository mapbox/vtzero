
#include <test.hpp>

#include <vtzero/geometry.hpp>

TEST_CASE("default constructed point") {
    const vtzero::point p{};

    REQUIRE(p.x == 0);
    REQUIRE(p.y == 0);
}

TEST_CASE("point") {
    const vtzero::point p1{4, 5};
    const vtzero::point p2{5, 4};
    const vtzero::point p3{4, 5};

    REQUIRE(p1.x == 4);
    REQUIRE(p1.y == 5);

    REQUIRE_FALSE(p1 == p2);
    REQUIRE(p1 != p2);
    REQUIRE(p1 == p3);
}

