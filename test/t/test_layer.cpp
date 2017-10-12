
#include <test.hpp>

#include <vtzero/vector_tile.hpp>
#include <vtzero/layer.hpp>

TEST_CASE("default constructed layer") {
    vtzero::layer layer{};
    REQUIRE_FALSE(layer.valid());
    REQUIRE_FALSE(layer);

    REQUIRE(layer.data() == vtzero::data_view{});
    REQUIRE_ASSERT(layer.version());
    REQUIRE_ASSERT(layer.extent());
    REQUIRE_ASSERT(layer.name());

    REQUIRE(layer.empty());
    REQUIRE(layer.num_features() == 0);

    REQUIRE_THROWS_AS(layer.key_table(), const assert_error&);
    REQUIRE_THROWS_AS(layer.value_table(), const assert_error&);

    REQUIRE_THROWS_AS(layer.key(0), const assert_error&);
    REQUIRE_THROWS_AS(layer.value(0), const assert_error&);
}

TEST_CASE("read a layer") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    auto layer = tile.get_layer_by_name("bridge");
    REQUIRE(layer.valid());
    REQUIRE(layer);

    REQUIRE(layer.version() == 1);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.name() == "bridge");

    REQUIRE_FALSE(layer.empty());
    REQUIRE(layer.num_features() == 2);

    const auto& kt = layer.key_table();
    REQUIRE(kt.size() == 4);
    REQUIRE(kt[0] == "class");

    const auto& vt = layer.value_table();
    REQUIRE(vt.size() == 4);
    REQUIRE(vt[0].type() == vtzero::property_value_type::string_value);
    REQUIRE(vt[0].string_value() == "main");
    REQUIRE(vt[1].type() == vtzero::property_value_type::int_value);
    REQUIRE(vt[1].int_value() == 0);

    REQUIRE(layer.key(0) == "class");
    REQUIRE(layer.key(1) == "oneway");
    REQUIRE(layer.key(2) == "osm_id");
    REQUIRE(layer.key(3) == "type");
    REQUIRE_THROWS_AS(layer.key(4), const std::out_of_range&);

    REQUIRE(layer.value(0).string_value() == "main");
    REQUIRE(layer.value(1).int_value() == 0);
    REQUIRE(layer.value(2).string_value() == "primary");
    REQUIRE(layer.value(3).string_value() == "tertiary");
    REQUIRE_THROWS_AS(layer.value(4), const std::out_of_range&);
}

