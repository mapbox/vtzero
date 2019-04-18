
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/feature.hpp>
#include <vtzero/layer.hpp>
#include <vtzero/types.hpp>
#include <vtzero/vector_tile.hpp>

#include <array>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

static const std::string types[] = { // NOLINT(cert-err58-cpp)
    "data_view", "uint", "sint", "double", "float", "true", "false", "null", "cstring", "string", "uint", "sint"
};

struct AttributeCheckHandler {

    std::size_t count = 0;
    std::size_t count_value = 0;

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        REQUIRE(count < sizeof(types) / sizeof(types[0]));
        REQUIRE(types[count] == std::string(key)); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        ++count;
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        ++count_value;
        if (count == 1) {
            REQUIRE(std::string{"foo"} == value);
        } else if (count == 9) {
            REQUIRE(std::string{"bar"} == value);
        } else if (count == 10) {
            REQUIRE(std::string{"baz"} == value);
        } else {
            REQUIRE(false);
        }
        return true;
    }

    bool attribute_value(uint64_t value, std::size_t /*depth*/) {
        ++count_value;
        if (count == 2) {
            REQUIRE(value == 17u);
        } else if (count == 11) {
            REQUIRE(value == std::numeric_limits<uint64_t>::max());
        } else {
            REQUIRE(false);;
        }
        return true;
    }

    bool attribute_value(int64_t value, std::size_t /*depth*/) {
        ++count_value;
        if (count == 3) {
            REQUIRE(value == -22);
        } else if (count == 12) {
            REQUIRE(value == std::numeric_limits<int64_t>::min());
        } else {
            REQUIRE(false);;
        }
        return true;
    }

    bool attribute_value(double value, std::size_t /*depth*/) {
        ++count_value;
        REQUIRE(count == 4);
        REQUIRE(value == Approx(17.22));
        return true;
    }

    bool attribute_value(float value, std::size_t /*depth*/) {
        ++count_value;
        REQUIRE(count == 5);
        REQUIRE(value == Approx(-5.3f));
        return true;
    }

    bool attribute_value(bool value, std::size_t /*depth*/) {
        ++count_value;
        if (count == 6) {
            REQUIRE(value);
        } else if (count == 7 || count == 8) { // count == 8 is only for the case where layer version == 2, because there is no "null" type in version 2
            REQUIRE_FALSE(value);
        } else {
            REQUIRE(false);
        }
        return true;
    }

    bool attribute_value(vtzero::null_type /*value*/, std::size_t /*depth*/) {
        ++count_value;
        REQUIRE(count == 8);
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) {
        ++count_value;
        REQUIRE(false);
        return false;
    }

    std::size_t result() const noexcept {
        REQUIRE(count == count_value);
        return count;
    }

}; // class AttributeCheckHandler

void test_scalar_attrs(uint32_t version) {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", version};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.add_scalar_attribute("data_view", vtzero::data_view{"foo"}); // 1
        fbuilder.add_scalar_attribute("uint", 17u); // 2
        fbuilder.add_scalar_attribute("sint", -22); // 3
        fbuilder.add_scalar_attribute("double", 17.22); // 4
        fbuilder.add_scalar_attribute("float", -5.3f); // 5
        fbuilder.add_scalar_attribute("true", true); // 6
        fbuilder.add_scalar_attribute("false", false); // 7
        fbuilder.add_scalar_attribute("null", vtzero::null_type{}); // 8
        fbuilder.add_scalar_attribute("cstring", "bar"); // 9
        fbuilder.add_scalar_attribute("string", std::string{"baz"}); // 10
        fbuilder.add_scalar_attribute("uint", std::numeric_limits<uint64_t>::max()); // 11
        fbuilder.add_scalar_attribute("sint", std::numeric_limits<int64_t>::min()); // 12
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == version);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    {
        AttributeCountHandler handler;
        const auto result = feature.decode_attributes(handler);
        REQUIRE(result.first == 12);
        REQUIRE(result.second == 12);
    }
    {
        AttributeCheckHandler handler;
        REQUIRE(feature.decode_attributes(handler) == 12);
    }
}

TEST_CASE("build feature with scalar attributes and read it again") {
    test_scalar_attrs(2);
    test_scalar_attrs(3);
}

TEST_CASE("build feature with list and map attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.add_scalar_attribute("some_int", 111u);
        fbuilder.start_list_attribute_with_key("list", 8);
        fbuilder.attribute_value(vtzero::data_view{"foo"}); // 1
        fbuilder.attribute_value(17u); // 2
        fbuilder.attribute_value(-22); // 3
        fbuilder.attribute_value(true); // 4
        fbuilder.attribute_value(false); // 5
        fbuilder.attribute_value(vtzero::null_type{}); // 6
        fbuilder.attribute_value("bar"); // 7
        fbuilder.attribute_value(std::string{"baz"}); // 8
        fbuilder.add_scalar_attribute("another_int", 222u);
        fbuilder.start_map_attribute_with_key("map", 2);
        fbuilder.add_scalar_attribute("x", 3); // 1
        fbuilder.add_scalar_attribute("y", 5); // 2
        fbuilder.add_scalar_attribute("a_different_int", 333u);
        fbuilder.commit();
    }

    const std::string expected{"some_int=uint(111)\nlist=list(8)[\nfoo\nuint(17)\nsint(-22)\ntrue\nfalse\nnull\nbar\nbaz\n]\nanother_int=uint(222)\nmap=map(2)[\nx=sint(3)\ny=sint(5)\n]\na_different_int=uint(333)\n"};

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    {
        AttributeCountHandler handler;
        const auto result = feature.decode_attributes(handler);
        REQUIRE(result.first == 7);
        REQUIRE(result.second == 15);
    }
    {
        AttributeDumpHandler handler;
        REQUIRE(feature.decode_attributes(handler) == expected);
    }
}

TEST_CASE("build feature with number-list attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    const auto index = lbuilder.add_attribute_scaling(vtzero::scaling{0, 2.0, 0.0});
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.start_number_list_with_key("nlist", 4, index);
        fbuilder.number_list_value(10);
        fbuilder.number_list_value(20);
        fbuilder.number_list_null_value();
        fbuilder.number_list_value(30);
        fbuilder.add_scalar_attribute("x", 3);
        fbuilder.commit();
    }

    const std::string expected{"nlist=number-list(4,0)[\n10\n20\nnull\n30\n]\nx=sint(3)\n"};

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.num_features() == 1);
    REQUIRE(layer.num_attribute_scalings() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    {
        AttributeCountHandler handler;
        const auto result = feature.decode_attributes(handler);
        REQUIRE(result.first == 2);
        REQUIRE(handler.count_number_list == 4);
    }
    {
        AttributeDumpHandler handler;
        REQUIRE(feature.decode_attributes(handler) == expected);
    }
}

TEST_CASE("build feature with list attribute from array and read it again") {
    const std::array<int, 8> pi = {{3, 1, 4, 1, 5, 9, 2, 6}};
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    SECTION("without size") {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.add_list_attribute("pi", pi.begin(), pi.end());
        fbuilder.commit();
    }

    SECTION("with size") {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.add_list_attribute("pi", pi.begin(), pi.end(), pi.size());
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    {
        AttributeDumpHandler handler;
        REQUIRE(feature.decode_attributes(handler) == "pi=list(8)[\nsint(3)\nsint(1)\nsint(4)\nsint(1)\nsint(5)\nsint(9)\nsint(2)\nsint(6)\n]\n");
    }
}

struct AttributeValuesTableCheckHandler {

    std::size_t count = 0;
    double value_sum = 0;
    std::string keys;

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        ++count;
        REQUIRE(key.size() == 1);
        keys += key.data()[0];
        return true;
    }

    bool attribute_value(double value, std::size_t /*depth*/) {
        value_sum += value;
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) {
        REQUIRE(false);
        return false;
    }

}; // class AttributeValuesTableCheckHandler

TEST_CASE("build version 3 feature with many double attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        int n = 0;
        char key[2] = "_";
        for (*key = 'a'; *key != 'z'; ++(*key), ++n) {
            fbuilder.add_scalar_attribute(key, 1.2 + n);
        }
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    {
        AttributeValuesTableCheckHandler handler;
        feature.decode_attributes(handler);
        REQUIRE(handler.count == 25);
        REQUIRE(handler.keys == "abcdefghijklmnopqrstuvwxy");
        const int sum_n = 25 * 24 / 2;
        REQUIRE(handler.value_sum == Approx(1.2 * 25 + sum_n));
    }
}

