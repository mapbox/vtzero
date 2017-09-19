
#include <vtzero/geometry.hpp>
#include <vtzero/builder.hpp>
#include <vtzero/tag_value.hpp>
#include <vtzero/value_view.hpp>

#include <catch.hpp>

TEST_CASE("string value") {
    vtzero::tag_value v{"foo"};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.string_value() == "foo");
}

TEST_CASE("float value") {
    vtzero::tag_value v{1.2f};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.float_value() == Approx(1.2));
}

TEST_CASE("double value") {
    vtzero::tag_value v{1.2};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.double_value() == Approx(1.2));
}

TEST_CASE("int value") {
    vtzero::tag_value v{vtzero::int_value_type{42}};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.int_value() == 42);
}

TEST_CASE("uint value") {
    vtzero::tag_value v{vtzero::uint_value_type{42}};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.uint_value() == 42);
}

TEST_CASE("sint value") {
    vtzero::tag_value v{vtzero::sint_value_type{42}};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.sint_value() == 42);
}

TEST_CASE("bool value") {
    vtzero::tag_value v{true};
    vtzero::value_view vv{v.data()};
    REQUIRE(vv.bool_value());
}

struct point_handler {

    std::vector<vtzero::point> data;

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

struct mypoint {
    int64_t p1;
    int64_t p2;
};

/*inline vtzero::point create_point(mypoint p) noexcept {
    return {static_cast<int32_t>(p.p1),
            static_cast<int32_t>(p.p2)};
}*/

namespace vtzero {

    template <>
    point create_point<::mypoint>(::mypoint p) noexcept {
        return {static_cast<int32_t>(p.p1),
                static_cast<int32_t>(p.p2)};
    }
}

TEST_CASE("Point builder") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder, 17};

    SECTION("add point using coordinates") {
        fbuilder.add_point(10, 20);
    }

    SECTION("add point using vtzero::point") {
        fbuilder.add_point(vtzero::point{10, 20});
    }

    SECTION("add point using mypoint") {
        fbuilder.add_point(mypoint{10, 20});
    }

    fbuilder.commit();

    std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 17);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

