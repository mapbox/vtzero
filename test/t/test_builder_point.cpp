
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

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

} // namespace vtzero

TEST_CASE("Point builder") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(17);

    SECTION("add point using coordinates / property using key/value") {
        fbuilder.add_point(10, 20);
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
    }

    SECTION("add point using vtzero::point / property using key/value") {
        fbuilder.add_point(vtzero::point{10, 20});
        fbuilder.add_property("foo", vtzero::encoded_property_value{22});
    }

    SECTION("add point using mypoint / property using property") {
        vtzero::encoded_property_value pv{3.5};
        vtzero::property p{"foo", vtzero::property_value{pv.data()}};
        fbuilder.add_point(mypoint{10, 20});
        fbuilder.add_property(p);
    }

    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature.id() == 17);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), handler);

    std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

