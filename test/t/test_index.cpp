
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>

#include <map>
#include <string>
#include <unordered_map>

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

    const std::string data = tbuilder.serialize();

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

