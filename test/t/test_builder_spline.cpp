
#include <test.hpp>
#include <test_point.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/builder_helper.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

using ls2d_type = std::vector<std::vector<test_point_2d>>;
using ls3d_type = std::vector<std::vector<test_point_3d>>;
using knot_type = std::vector<int64_t>;

struct spline_handler_2d {

    constexpr static const int dimensions = 2;
    constexpr static const unsigned int max_geometric_attributes = 0;

    ls2d_type data;
    knot_type knots;

    static test_point_2d convert(const vtzero::point_2d& p) noexcept {
        return {p.x, p.y};
    }

    void controlpoints_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void controlpoints_point(const test_point_2d point) {
        data.back().push_back(point);
    }

    void controlpoints_end() const noexcept {
    }

    void knots_begin(const uint32_t count) noexcept {
        knots.reserve(count);
    }

    void knots_value(const int64_t value) noexcept {
        knots.push_back(value);
    }

    void knots_end() noexcept {
    }

}; // struct spline_handler_2d

struct spline_handler_3d {

    constexpr static const int dimensions = 3;
    constexpr static const unsigned int max_geometric_attributes = 0;

    ls3d_type data;

    static test_point_3d convert(const vtzero::point_3d& p) noexcept {
        return {p.x, p.y, p.z};
    }

    void controlpoints_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void controlpoints_point(const test_point_3d point) {
        data.back().push_back(point);
    }

    void controlpoints_end() const noexcept {
    }

    void knots_begin(const uint32_t /*count*/) noexcept {
    }

    void knots_value(const int64_t /*value*/) noexcept {
    }

    void knots_end() noexcept {
    }

}; // struct spline_handler_3d

static void test_spline_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    {
        vtzero::spline_feature_builder<2> fbuilder{lbuilder};

        if (with_id) {
            fbuilder.set_integer_id(17);
        }

        fbuilder.add_spline(3, 0);
        fbuilder.set_point(vtzero::point_2d{10, 20});
        fbuilder.set_point(vtzero::point_2d{20, 30});
        fbuilder.set_point(vtzero::point_2d{30, 40});
        fbuilder.set_knot(1);
        fbuilder.set_knot(2);
        fbuilder.set_knot(3);
        fbuilder.set_knot(4);
        fbuilder.set_knot(5);
        fbuilder.set_knot(6);

        if (with_attr) {
            fbuilder.add_scalar_attribute("foo", "bar");
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
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    spline_handler_2d handler;
    feature.decode_spline_geometry(handler);

    const ls2d_type result = {{{10, 20}, {20, 30}, {30, 40}}};
    REQUIRE(handler.data == result);
}

TEST_CASE("spline builder without id/without attributes") {
    test_spline_builder(false, false);
}

TEST_CASE("spline builder without id/with attributes") {
    test_spline_builder(false, true);
}

TEST_CASE("spline builder with id/without attributes") {
    test_spline_builder(true, false);
}

TEST_CASE("spline builder with id/with attributes") {
    test_spline_builder(true, true);
}

TEST_CASE("Calling add_spline() with bad values throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    SECTION("0") {
        REQUIRE_THROWS_AS(fbuilder.add_spline(0, 0), const assert_error&);
    }
    SECTION("1") {
        REQUIRE_THROWS_AS(fbuilder.add_spline(1, 0), const assert_error&);
    }
    SECTION("2^29") {
        REQUIRE_THROWS_AS(fbuilder.add_spline(1ul << 29u, 0), const assert_error&);
    }
}

static void test_multispline_builder(const bool with_id, const bool with_attr) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    if (with_id) {
        fbuilder.set_integer_id(17);
    }

    fbuilder.add_spline(3, 0);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 30});
    fbuilder.set_point(vtzero::point_2d{30, 40});
    fbuilder.set_knot(1);
    fbuilder.set_knot(2);
    fbuilder.set_knot(3);
    fbuilder.set_knot(4);
    fbuilder.set_knot(5);
    fbuilder.set_knot(6);

    fbuilder.add_spline(2, 0);
    fbuilder.set_point(vtzero::point_2d{1, 2});
    fbuilder.set_point(vtzero::point_2d{2, 1});
    fbuilder.set_knot(1);
    fbuilder.set_knot(2);
    fbuilder.set_knot(3);
    fbuilder.set_knot(4);
    fbuilder.set_knot(5);

    if (with_attr) {
        fbuilder.add_scalar_attribute("foo", "bar");
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == (with_id ? 17 : 0));

    spline_handler_2d handler;
    feature.decode_spline_geometry(handler);

    const ls2d_type geom_result = {{{10, 20}, {20, 30}, {30, 40}}, {{1, 2}, {2, 1}}};
    const knot_type knot_result = {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5};
    REQUIRE(handler.data == geom_result);
    REQUIRE(handler.knots == knot_result);
}

TEST_CASE("Multispline builder without id/without attributes") {
    test_multispline_builder(false, false);
}

TEST_CASE("Multispline builder without id/with attributes") {
    test_multispline_builder(false, true);
}

TEST_CASE("Multispline builder with id/without attributes") {
    test_multispline_builder(true, false);
}

TEST_CASE("Multispline builder with id/with attributes") {
    test_multispline_builder(true, true);
}

TEST_CASE("Calling add_spline() twice throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_spline(3, 0);
    REQUIRE_THROWS_AS(fbuilder.add_spline(2, 0), const assert_error&);
}

TEST_CASE("Calling spline_feature_builder<2>::set_point() throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
}

TEST_CASE("Calling spline_feature_builder<2>::set_point() with same point throws") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_spline(2, 0);
    fbuilder.set_point(vtzero::point_2d{10, 10});
    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d(10, 10)), const vtzero::geometry_exception&);
}

TEST_CASE("Calling spline_feature_builder<2>::set_point() too often throws assert") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::spline_feature_builder<2> fbuilder{lbuilder};

    fbuilder.add_spline(2, 0);
    fbuilder.set_point(vtzero::point_2d{10, 20});
    fbuilder.set_point(vtzero::point_2d{20, 20});
    REQUIRE_THROWS_AS(fbuilder.set_point(vtzero::point_2d{}), const assert_error&);
}

TEST_CASE("Adding several splines with feature rollback in the middle") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    {
        vtzero::spline_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_spline(2, 0);
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.set_point(vtzero::point_2d{20, 20});
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.commit();
    }

    try {
        vtzero::spline_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(2);
        fbuilder.add_spline(2, 0);
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.commit();
    } catch (vtzero::geometry_exception&) {
    }

    {
        vtzero::spline_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(3);
        fbuilder.add_spline(2, 0);
        fbuilder.set_point(vtzero::point_2d{10, 20});
        fbuilder.set_point(vtzero::point_2d{20, 10});
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.set_knot(1);
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.num_features() == 2);

    auto it = layer.begin();
    auto feature = *it++;
    REQUIRE(feature.id() == 1);
    feature = *it;
    REQUIRE(feature.id() == 3);
}

