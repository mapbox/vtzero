
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/feature.hpp>
#include <vtzero/layer.hpp>
#include <vtzero/types.hpp>
#include <vtzero/vector_tile.hpp>

#include <string>
#include <utility>

struct AttributeCountHandler {

    int count_ki = 0;
    int count_k = 0;
    int count_v = 0;
    int count_nonscalar = 0;
    std::size_t count_number_list = 0;

    bool key_index(vtzero::index_value /*ki*/, std::size_t /*depth*/) noexcept {
        ++count_ki;
        return true;
    }

    bool attribute_key(vtzero::data_view /*key*/, std::size_t /*depth*/) noexcept {
        ++count_k;
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) noexcept {
        ++count_v;
        return true;
    }

    bool start_list_attribute(std::size_t /*size*/, std::size_t /*depth*/) noexcept {
        ++count_nonscalar;
        ++count_v;
        return true;
    }

    bool end_list_attribute(std::size_t /*depth*/) noexcept {
        --count_nonscalar;
        return true;
    }

    bool start_map_attribute(std::size_t /*size*/, std::size_t /*depth*/) noexcept {
        ++count_nonscalar;
        ++count_v;
        return true;
    }

    bool end_map_attribute(std::size_t /*depth*/) noexcept {
        --count_nonscalar;
        return true;
    }

    bool start_number_list(std::size_t count, vtzero::index_value index, std::size_t /*depth*/) noexcept {
        REQUIRE(index.value() == 0);
        count_number_list += count;
        return true;
    }

    std::pair<int, int> result() const {
        REQUIRE(count_nonscalar == 0);
        REQUIRE(count_ki == count_k);
        return std::make_pair(count_k, count_v);
    }

}; // class AttributeCountHandler

static const std::string types[] = { // NOLINT(cert-err58-cpp)
    "data_view", "uint", "sint", "double", "float", "true", "false", "null", "cstring", "string"
};

struct AttributeCheckHandler {

    std::size_t count = 0;

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        REQUIRE(count < sizeof(types));
        REQUIRE(types[count] == std::string(key)); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        ++count;
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
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
        REQUIRE(count == 2);
        REQUIRE(value == 17u);
        return true;
    }

    bool attribute_value(int64_t value, std::size_t /*depth*/) {
        REQUIRE(count == 3);
        REQUIRE(value == -22);
        return true;
    }

    bool attribute_value(double value, std::size_t /*depth*/) {
        REQUIRE(count == 4);
        REQUIRE(value == Approx(17.22));
        return true;
    }

    bool attribute_value(float value, std::size_t /*depth*/) {
        REQUIRE(count == 5);
        REQUIRE(value == Approx(-5.3f));
        return true;
    }

    bool attribute_value(bool value, std::size_t /*depth*/) {
        if (count == 6) {
            REQUIRE(value);
        } else if (count == 7) {
            REQUIRE_FALSE(value);
        } else {
            REQUIRE(false);
        }
        return true;
    }

    bool attribute_value(vtzero::null_type /*value*/, std::size_t /*depth*/) {
        REQUIRE(count == 8);
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) {
        REQUIRE(false);
        return false;
    }

    std::size_t result() const noexcept {
        return count;
    }

}; // class AttributeCheckHandler

TEST_CASE("build feature with scalar attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 20);
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
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature);
    REQUIRE(feature.id() == 1);

    {
        AttributeCountHandler handler;
        const auto result = feature.decode_attributes(handler);
        REQUIRE(result.first == 10);
        REQUIRE(result.second == 10);
    }
    {
        AttributeCheckHandler handler;
        REQUIRE(feature.decode_attributes(handler) == 10);
    }
}

struct AttributeDumpHandler {

    std::string out{};

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        out.append(key.data(), key.size());
        out += '=';
        return true;
    }

    bool start_list_attribute(std::size_t size, std::size_t /*depth*/) {
        out += "list(";
        out += std::to_string(size);
        out += ")[\n";
        return true;
    }

    bool end_list_attribute(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool start_map_attribute(std::size_t size, std::size_t /*depth*/) {
        out += "map(";
        out += std::to_string(size);
        out += ")[\n";
        return true;
    }

    bool start_number_list(std::size_t size, vtzero::index_value index, std::size_t /*depth*/) {
        out += "number-list(";
        out += std::to_string(size);
        out += ',';
        out += std::to_string(index.value());
        out += ")[\n";
        return true;
    }

    bool number_list_value(uint64_t value, std::size_t /*depth*/) {
        out += std::to_string(value);
        out += '\n';
        return true;
    }

    bool end_number_list(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool end_map_attribute(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        out.append(value.data(), value.size());
        out += '\n';
        return true;
    }

    bool attribute_value(bool value, std::size_t /*depth*/) {
        out += (value ? "true\n" : "false\n");
        return true;
    }

    bool attribute_value(vtzero::null_type /*value*/, std::size_t /*depth*/) {
        out += "null\n";
        return true;
    }

    template <typename T>
    bool attribute_value(T value, std::size_t /*depth*/) {
        out += std::to_string(value);
        out += '\n';
        return true;
    }

    std::string result() const noexcept {
        return out;
    }

}; // class AttributeDumpHandler

TEST_CASE("build feature with list and map attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 20);
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

    std::string expected{"some_int=111\nlist=list(8)[\nfoo\n17\n-22\ntrue\nfalse\nnull\nbar\nbaz\n]\nanother_int=222\nmap=map(2)[\nx=3\ny=5\n]\na_different_int=333\n"};

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature);
    REQUIRE(feature.id() == 1);

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

TEST_CASE("build feature with number list attributes and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    const auto index = lbuilder.add_attribute_scaling(vtzero::scaling{0, 2.0, 0.0});
    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 20);
        fbuilder.start_number_list_with_key("nlist", 3, index);
        fbuilder.number_list_value(10);
        fbuilder.number_list_value(20);
        fbuilder.number_list_value(30);
        fbuilder.add_scalar_attribute("x", 3);
        fbuilder.commit();
    }

    std::string expected{"nlist=number-list(3,0)[\n10\n20\n30\n]\nx=3\n"};

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.num_features() == 1);
    REQUIRE(layer.num_attribute_scalings() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature);
    REQUIRE(feature.id() == 1);

    {
        AttributeCountHandler handler;
        const auto result = feature.decode_attributes(handler);
        REQUIRE(result.first == 2);
        REQUIRE(handler.count_number_list == 3);
    }
    {
        AttributeDumpHandler handler;
        REQUIRE(feature.decode_attributes(handler) == expected);
    }
}

TEST_CASE("build feature with list property from array and read it again") {
    const std::array<int, 8> pi = {{3, 1, 4, 1, 5, 9, 2, 6}};
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};

    SECTION("without size") {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 20);
        fbuilder.add_list_attribute("pi", pi.begin(), pi.end());
        fbuilder.commit();
    }

    SECTION("with size") {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 20);
        fbuilder.add_list_attribute("pi", pi.begin(), pi.end(), pi.size());
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = layer.next_feature();
    REQUIRE(feature);
    REQUIRE(feature.id() == 1);

    {
        AttributeDumpHandler handler;
        REQUIRE(feature.decode_attributes(handler) == "pi=list(8)[\n3\n1\n4\n1\n5\n9\n2\n6\n]\n");
    }
}

