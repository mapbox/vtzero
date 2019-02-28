
#include <test.hpp>
#include <test_point.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/builder_helper.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <vector>

using polygon_type = std::vector<std::vector<test_point_2d>>;

struct polygon_handler {

    constexpr static const int dimensions = 2;
    constexpr static const unsigned int max_geometric_attributes = 0;

    polygon_type data;

    static test_point_2d convert(const vtzero::point_2d& p) noexcept {
        return {p.x, p.y};
    }

    void ring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void ring_point(const test_point_2d point) {
        data.back().push_back(point);
    }

    void ring_end(vtzero::ring_type /* type */) const noexcept {
    }

}; // struct polygon_handler

static void test_polygon_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        fbuilder.add_ring(4);
        fbuilder.set_point(vtzero::point_2d{10, 20});
        fbuilder.set_point(vtzero::point_2d{20, 30});
        fbuilder.set_point(vtzero::point_2d{30, 40});
        fbuilder.set_point(vtzero::point_2d{10, 20});

        if (with_attr) {
            fbuilder.add_property("foo", "bar");
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.integer_id() == (with_id ? 17 : 0));

    polygon_handler handler;
    feature.decode_polygon_geometry(handler);

    const polygon_type result = {{{10, 20}, {20, 30}, {30, 40}, {10, 20}}};
    REQUIRE(handler.data == result);
}

TEST_CASE("polygon builder without id/without attributes") {
    test_polygon_builder(false, false);
}

TEST_CASE("polygon builder without id/with attributes") {
    test_polygon_builder(false, true);
}

TEST_CASE("polygon builder with id/without attributes") {
    test_polygon_builder(true, false);
}

TEST_CASE("polygon builder with id/with attributes") {
    test_polygon_builder(true, true);
}

TEST_CASE("Calling add_ring() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_ring(0), const assert_error&);
    }
    SECTION("1") {
        REQUIRE_THROWS_AS(fbuilder.add_ring(1), const assert_error&);
    }
    SECTION("2") {
        REQUIRE_THROWS_AS(fbuilder.add_ring(2), const assert_error&);
    }
    SECTION("3") {
        REQUIRE_THROWS_AS(fbuilder.add_ring(3), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_ring(1ul << 29u), const assert_error&);
    }
}

static void test_multipolygon_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_integer_id(17);
    }

    fbuilder.add_ring(4);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 30});
    fbuilder.set_point(vtzero::point_2d{30, 40});
    fbuilder.set_point(vtzero::point_2d{10, 20});

    fbuilder.add_ring(5);
    fbuilder.set_point(vtzero::point_2d{1, 1});
    fbuilder.set_point(vtzero::point_2d{2, 1});
    fbuilder.set_point(vtzero::point_2d{2, 2});
    fbuilder.set_point(vtzero::point_2d{1, 2});

    if (with_id) {
        fbuilder.set_point(vtzero::point_2d{1, 1});
    } else {
        fbuilder.close_ring();
    }

    if (with_attr) {
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.integer_id() == (with_id ? 17 : 0));

    polygon_handler handler;
    feature.decode_polygon_geometry(handler);

    const polygon_type result = {{{10, 20}, {20, 30}, {30, 40}, {10, 20}},
                                 {{1, 1}, {2, 1}, {2, 2}, {1, 2}, {1, 1}}};
    REQUIRE(handler.data == result);
}


TEST_CASE("Multipolygon builder without id/without attributes") {
    test_multipolygon_builder(false, false);
}

TEST_CASE("Multipolygon builder without id/with attributes") {
    test_multipolygon_builder(false, true);
}

TEST_CASE("Multipolygon builder with id/without attributes") {
    test_multipolygon_builder(true, false);
}

TEST_CASE("Multipolygon builder with id/with attributes") {
    test_multipolygon_builder(true, true);
}

TEST_CASE("Calling add_ring() twice throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_ring(4);
    REQUIRE_THROWS_AS(fbuilder.add_ring(4), const assert_error&);
}

TEST_CASE("Calling polygon_feature_builder<2>::set_point()/close_ring() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    SECTION("set_point") {
        REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
    }
    SECTION("close_ring") {
        REQUIRE_THROWS_AS(fbuilder.close_ring(), const assert_error&);
    }
}

TEST_CASE("Calling polygon_feature_builder<2>::set_point()/close_ring() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_ring(4);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 20});
    fbuilder.set_point(vtzero::point_2d{30, 20});
    fbuilder.set_point(vtzero::point_2d{10, 20});

    SECTION("set_point") {
        REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
    }
    SECTION("close_ring") {
        REQUIRE_THROWS_AS(fbuilder.close_ring(), const assert_error&);
    }
}

TEST_CASE("Calling polygon_feature_builder<2>::set_point() with same point throws") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_ring(4);
    fbuilder.set_point(vtzero::point_2d{10, 10});
    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d(10, 10)), const vtzero::geometry_exception&);
}

TEST_CASE("Calling polygon_feature_builder<2>::set_point() creating unclosed ring throws") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_ring(4);
    fbuilder.set_point(vtzero::point_2d{10, 10});
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 20});
    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const vtzero::geometry_exception&);
}

TEST_CASE("Add polygon from container") {
    const polygon_type expected = {{{10, 20}, {20, 30}, {30, 40}, {10, 20}}};
    const std::vector<std::vector<vtzero::point_2d>> points = {{{10, 20}, {20, 30}, {30, 40}, {10, 20}}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::polygon_feature_builder<2> fbuilder{lbuilder};
    vtzero::add_ring_from_container(points[0], fbuilder);
    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();

    polygon_handler handler;
    feature.decode_polygon_geometry(handler);

    REQUIRE(handler.data == expected);
}

