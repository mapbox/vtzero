
#include <test.hpp>
#include <test_geometry_handler.hpp>
#include <test_point.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/builder_helper.hpp>
#include <vtzero/detail/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <vector>

static void test_point_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    REQUIRE(lbuilder.estimated_size() == 25);
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        SECTION("add point using point_2d / property using key/value") {
            fbuilder.add_point(vtzero::point_2d{10, 20});
            if (with_attr) {
                fbuilder.add_property("foo", vtzero::encoded_property_value{22});
            }
        }

        SECTION("add point using point_2d / property using property") {
            vtzero::encoded_property_value pv{3.5};
            vtzero::property p{"foo", vtzero::property_value{pv.data()}};
            fbuilder.add_point(vtzero::point_2d{10, 20});
            if (with_attr) {
                fbuilder.add_property(p);
            }
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

    point_handler<2> handler;
    feature.decode_point_geometry(handler);

    const point_handler<2>::result_type expected = {{10, 20}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Point builder without id/without attributes") {
    test_point_builder(false, false);
}

TEST_CASE("Point builder without id/with attributes") {
    test_point_builder(false, true);
}

TEST_CASE("Point builder with id/without attributes") {
    test_point_builder(true, false);
}

TEST_CASE("Point builder with id/with attributes") {
    test_point_builder(true, true);
}

static void test_point_builder_vt3(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        SECTION("add point using point_2d / property using key/string value") {
            fbuilder.add_point(vtzero::point_2d{10, 20});
            if (with_attr) {
                fbuilder.add_scalar_attribute("foo", vtzero::data_view{"bar"});
            }
        }

        SECTION("add point using point_2d / property using key/int value") {
            fbuilder.add_point(vtzero::point_2d{10, 20});
            if (with_attr) {
                fbuilder.add_scalar_attribute("foo", 22);
            }
        }

        SECTION("add point using point_2d / property using key/double value") {
            fbuilder.add_point(vtzero::point_2d{10, 20});
            if (with_attr) {
                fbuilder.add_scalar_attribute("foo", 3.5);
            }
        }

        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.integer_id() == (with_id ? 17 : 0));
    REQUIRE_FALSE(feature.has_3d_geometry());

    point_handler<2> handler;
    feature.decode_point_geometry(handler);

    const std::vector<vtzero::point_2d> expected = {{10, 20}};
    REQUIRE(handler.data == expected);
}

TEST_CASE("Point builder without id/without attributes (vt3)") {
    test_point_builder_vt3(false, false);
}

TEST_CASE("Point builder without id/with attributes (vt3)") {
    test_point_builder_vt3(false, true);
}

TEST_CASE("Point builder with id/without attributes (vt3)") {
    test_point_builder_vt3(true, false);
}

TEST_CASE("Point builder with id/with attributes (vt3)") {
    test_point_builder_vt3(true, true);
}

TEST_CASE("Point builder with 3d point") {
    vtzero::scaling scaling{10, 1.0, 3.0};
    const auto elev = scaling.encode32(30.0);

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    lbuilder.set_elevation_scaling(scaling);
    REQUIRE(lbuilder.elevation_scaling() == scaling);
    {
        vtzero::point_feature_builder<3> fbuilder{lbuilder};
        fbuilder.set_integer_id(17);
        fbuilder.add_point(vtzero::point_3d{10, 20, elev});
        fbuilder.commit();
    }
    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.integer_id() == 17);
    REQUIRE(feature.has_3d_geometry());

    point_handler<3> handler;
    feature.decode_point_geometry(handler);
    REQUIRE(handler.data.size() == 1);

    const auto p = handler.data[0];
    REQUIRE(p.x == 10);
    REQUIRE(p.y == 20);
    REQUIRE(layer.elevation_scaling() == scaling);
    REQUIRE(layer.elevation_scaling().decode(p.z) == Approx(30.0));
}

TEST_CASE("Calling add_points() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_points(0), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_points(1ul << 29u), const assert_error&);
    }
}

static void test_multipoint_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_integer_id(17);
    }

    fbuilder.add_points(3);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 30});
    fbuilder.set_point(vtzero::point_2d{30, 40});

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

    point_handler<2> handler;
    feature.decode_point_geometry(handler);

    const point_handler<2>::result_type expected = {{10, 20}, {20, 30}, {30, 40}};
    REQUIRE(handler.data == expected);
}


TEST_CASE("Multipoint builder without id/without attributes") {
    test_multipoint_builder(false, false);
}

TEST_CASE("Multipoint builder without id/with attributes") {
    test_multipoint_builder(false, true);
}

TEST_CASE("Multipoint builder with id/without attributes") {
    test_multipoint_builder(true, false);
}

TEST_CASE("Multipoint builder with id/with attributes") {
    test_multipoint_builder(true, true);
}

TEST_CASE("Calling add_point() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_point(vtzero::point_2d{10, 20});

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(vtzero::point_2d{10, 20}), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
    SECTION("set_point()") {
        REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
    }
}

TEST_CASE("Calling point_feature_builder<2>::set_point() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
}

TEST_CASE("Calling add_points() and then other geometry functions throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_points(2);

    SECTION("add_point()") {
        REQUIRE_THROWS_AS(fbuilder.add_point(vtzero::point_2d{10, 20}), const assert_error&);
    }
    SECTION("add_points()") {
        REQUIRE_THROWS_AS(fbuilder.add_points(2), const assert_error&);
    }
}

TEST_CASE("Calling point_feature_builder<2>::set_point() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_points(2);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 20});
    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
}

TEST_CASE("Add points from container") {
    const point_handler<2>::result_type points = {{10, 20}, {20, 30}, {30, 40}};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder<2> fbuilder{lbuilder};
    vtzero::add_points_from_container(points, fbuilder);
    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();

    point_handler<2> handler;
    feature.decode_point_geometry(handler);

    REQUIRE(handler.data == points);
}

