
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename T>
struct movable_not_copyable {
    constexpr static bool value = !std::is_copy_constructible<T>::value &&
                                  !std::is_copy_assignable<T>::value    &&
                                   std::is_nothrow_move_constructible<T>::value &&
                                   std::is_nothrow_move_assignable<T>::value;
};

static_assert(movable_not_copyable<vtzero::tile_builder>::value, "tile_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::point_feature_builder>::value, "point_feature_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::linestring_feature_builder>::value, "linestring_feature_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::polygon_feature_builder>::value, "polygon_feature_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::geometry_feature_builder>::value, "geometry_feature_builder should be nothrow movable, but not copyable");

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

TEST_CASE("Create tile from existing layers") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    SECTION("add_existing_layer(layer)") {
        while (auto layer = tile.next_layer()) {
            tbuilder.add_existing_layer(layer);
        }
    }

    SECTION("add_existing_layer(data_view)") {
        while (auto layer = tile.next_layer()) {
            tbuilder.add_existing_layer(layer.data());
        }
    }

    std::string data = tbuilder.serialize();

    REQUIRE(data == buffer);
}

TEST_CASE("Create layer based on existing layer") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};
    const auto layer = tile.get_layer_by_name("place_label");

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, layer};
    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(42);
    fbuilder.add_point(10, 20);
    fbuilder.commit();

    std::string data = tbuilder.serialize();
    vtzero::vector_tile new_tile{data};
    const auto new_layer = new_tile.next_layer();
    REQUIRE(std::string(new_layer.name()) == "place_label");
    REQUIRE(new_layer.version() == 1);
    REQUIRE(new_layer.extent() == 4096);
}

TEST_CASE("Create layer and add keys/values") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "name"};

    const auto ki1 = lbuilder.add_key_without_dup_check("key1");
    const auto ki2 = lbuilder.add_key("key2");
    const auto ki3 = lbuilder.add_key("key1");

    REQUIRE(ki1.value() != ki2.value());
    REQUIRE(ki1.value() == ki3.value());

    const auto vi1 = lbuilder.add_value_without_dup_check(vtzero::encoded_property_value{"value1"});
    vtzero::encoded_property_value value2{"value2"};
    const auto vi2 = lbuilder.add_value_without_dup_check(vtzero::property_value{value2.data()});

    const auto vi3 = lbuilder.add_value(vtzero::encoded_property_value{"value1"});
    const auto vi4 = lbuilder.add_value(vtzero::encoded_property_value{19});
    const auto vi5 = lbuilder.add_value(vtzero::encoded_property_value{19.0});
    const auto vi6 = lbuilder.add_value(vtzero::encoded_property_value{22});
    vtzero::encoded_property_value nineteen{19};
    const auto vi7 = lbuilder.add_value(vtzero::property_value{nineteen.data()});

    REQUIRE(vi1.value() != vi2.value());
    REQUIRE(vi1.value() == vi3.value());
    REQUIRE(vi1.value() != vi4.value());
    REQUIRE(vi1.value() != vi5.value());
    REQUIRE(vi1.value() != vi6.value());
    REQUIRE(vi4.value() != vi5.value());
    REQUIRE(vi4.value() != vi6.value());
    REQUIRE(vi4.value() == vi7.value());
}

TEST_CASE("Point builder") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(17);

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

    auto layer = tile.next_layer();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature.id() == 17);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

TEST_CASE("value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    const auto key = lbuilder.add_key("some_key");

    vtzero::point_feature_builder fbuilder{lbuilder};
    fbuilder.set_id(17);
    fbuilder.add_point(10, 20);

    SECTION("no index") {
        fbuilder.add_property(key, vtzero::sint_value_type{12});
    }

    SECTION("external value index using unordered_map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("external value index using map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("property_value_type index") {
        vtzero::value_index_internal<std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(vtzero::encoded_property_value{vtzero::sint_value_type{12}}));
    }

    fbuilder.commit();

    std::string data = tbuilder.serialize();

    // ============

    vtzero::vector_tile tile{data};

    REQUIRE(tile.count_layers() == 1);
    auto layer = tile.next_layer();
    REQUIRE(layer.num_features() == 1);
    auto feature = layer.next_feature();
    REQUIRE(feature.id() == 17);
    const auto property = feature.next_property();
    REQUIRE(property.value().sint_value() == 12);
}

