
#include <test.hpp>

#include <vtzero/tile.hpp>

TEST_CASE("z0") {
    vtzero::tile z0{0, 0, 0};

    vtzero::point pmin{0, 0};
    vtzero::point pmax{4096, 4096};

    REQUIRE(z0.transform<mypoint>(pmin) == mypoint{-2003750834, -2003750834});
    REQUIRE(z0.transform<mypoint>(pmax) == mypoint{ 2003750834,  2003750834});
}

TEST_CASE("z4") {
    vtzero::tile z4_0{0, 0, 4};
    vtzero::tile z4_15{15, 15, 4};

    vtzero::point pmin{0, 0};
    vtzero::point pmax{4096, 4096};

    REQUIRE(z4_0.transform<mypoint>(pmin) == mypoint{-2003750834, -2003750834});
    REQUIRE(z4_15.transform<mypoint>(pmax) == mypoint{ 2003750834,  2003750834});
}

