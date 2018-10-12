
#include <test.hpp>

#include <vtzero/tile.hpp>

static_assert(vtzero::detail::num_tiles_in_zoom( 0) ==       1, "wrong number of tiles");
static_assert(vtzero::detail::num_tiles_in_zoom( 1) ==       2, "wrong number of tiles");
static_assert(vtzero::detail::num_tiles_in_zoom( 2) ==       4, "wrong number of tiles");
static_assert(vtzero::detail::num_tiles_in_zoom(10) ==    1024, "wrong number of tiles");
static_assert(vtzero::detail::num_tiles_in_zoom(20) == 1048576, "wrong number of tiles");

TEST_CASE("Tile") {
    vtzero::tile t1;
    REQUIRE(t1.x() == 0);
    REQUIRE(t1.y() == 0);
    REQUIRE(t1.zoom() == 0);
    REQUIRE(t1.extent() == 4096);

    vtzero::tile t2{1, 2, 3};
    REQUIRE(t2.x() == 1);
    REQUIRE(t2.y() == 2);
    REQUIRE(t2.zoom() == 3);
    REQUIRE(t2.extent() == 4096);

    vtzero::tile t3{0, 200, 12, 8192};
    REQUIRE(t3.x() == 0);
    REQUIRE(t3.y() == 200);
    REQUIRE(t3.zoom() == 12);
    REQUIRE(t3.extent() == 8192);
}

TEST_CASE("Tile: get assert if x or y is out of range") {
    REQUIRE_ASSERT(vtzero::tile(0, 1, 0));
    REQUIRE_ASSERT(vtzero::tile(1, 0, 0));
    REQUIRE_ASSERT(vtzero::tile(1, 1, 0));
    REQUIRE_ASSERT(vtzero::tile(16, 0, 4));
    REQUIRE_ASSERT(vtzero::tile(0, 16, 4));
}

TEST_CASE("Tile: max extent") {
    for (uint32_t zoom = 0; zoom < 16; ++zoom) {
        const auto maxtile = vtzero::detail::num_tiles_in_zoom(zoom) - 1;

        const vtzero::tile min{      0,       0, zoom};
        const vtzero::tile max{maxtile, maxtile, zoom};

        const vtzero::point pmin{   0,    0};
        const vtzero::point pmax{4096, 4096};

        const auto rmin = min.transform_int(pmin);
        const auto rmax = max.transform_int(pmax);

        REQUIRE(rmin.first  == -20037508342);
        REQUIRE(rmin.second == -20037508342);
        REQUIRE(rmax.first  ==  20037508342);
        REQUIRE(rmax.second ==  20037508342);

        const auto dmin = min.transform_double(pmin);
        const auto dmax = max.transform_double(pmax);

        REQUIRE(dmin.first  == Approx(-20037508.342));
        REQUIRE(dmin.second == Approx(-20037508.342));
        REQUIRE(dmax.first  == Approx( 20037508.342));
        REQUIRE(dmax.second == Approx( 20037508.342));
    }
}

