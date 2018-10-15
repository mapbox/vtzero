
#include <test.hpp>

#include <vtzero/feature.hpp>
#include <vtzero/layer.hpp>
#include <vtzero/vector_tile.hpp>

#include <map>
#include <string>
#include <utility>

TEST_CASE("default constructed feature") {
    vtzero::feature feature{};

    REQUIRE_FALSE(feature.valid());
    REQUIRE_FALSE(feature);
    REQUIRE(feature.id() == 0);
    REQUIRE_FALSE(feature.has_id());
    REQUIRE_FALSE(feature.has_integer_id());
    REQUIRE_FALSE(feature.has_string_id());
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
    REQUIRE_FALSE(feature.has_attributes());
}

TEST_CASE("read a feature") {
    const auto data = load_test_tile();
    const vtzero::vector_tile tile{data};

    const auto layer = tile.get_layer_by_name("bridge");
    REQUIRE(layer.valid());

    const auto feature = *layer.begin();
    REQUIRE(feature.valid());
    REQUIRE(feature);
    REQUIRE(feature.id() == 0);
    REQUIRE(feature.has_id());
    REQUIRE(feature.has_integer_id());
    REQUIRE_FALSE(feature.has_string_id());
    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);
    REQUIRE(feature.has_attributes());
}

TEST_CASE("iterate over all attributes of a feature") {
    const auto data = load_test_tile();
    const vtzero::vector_tile tile{data};
    const auto layer = tile.get_layer_by_name("bridge");
    const auto feature = *layer.begin();

    const std::string expected{"class=main\noneway=sint(0)\nosm_id=sint(0)\ntype=primary\n"};
    AttributeDumpHandler handler;
    REQUIRE(feature.decode_attributes(handler) == expected);
}

struct PropertyHandler {

    int count_ki = 0;
    int count_k = 0;
    int count_vi = 0;
    int count_v = 0;

    bool key_is_type = false;

    void key_index(vtzero::index_value /*ki*/, std::size_t /*depth*/) noexcept {
        ++count_ki;
    }

    void value_index(vtzero::index_value /*vi*/, std::size_t /*depth*/) noexcept {
        ++count_vi;
    }

    void attribute_key(vtzero::data_view key, std::size_t /*depth*/) noexcept {
        ++count_k;
        if (key == "type") {
            key_is_type = true;
        }
    }

    void attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        ++count_v;
        if (key_is_type) {
            REQUIRE(value == "primary");
            key_is_type = false;
        }
    }

}; // class PropertyHandler

TEST_CASE("decode properties of a feature") {
    const auto data = load_test_tile();
    const vtzero::vector_tile tile{data};
    const auto layer = tile.get_layer_by_name("bridge");
    const auto feature = *layer.begin();
    REQUIRE(feature);

    PropertyHandler handler;
    feature.decode_attributes(handler);
    REQUIRE(handler.count_ki == 4);
    REQUIRE(handler.count_k == 4);
    REQUIRE(handler.count_vi == 4);
    REQUIRE(handler.count_v == 2);
}

class MapHandler {

    std::map<std::string, std::string> m_map;
    vtzero::data_view m_key;

public:

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) noexcept {
        m_key = key;
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        m_map[std::string(m_key)] = std::string(value);
        return true;
    }

    std::map<std::string, std::string> result() {
        std::map<std::string, std::string> result;
        using std::swap;
        swap(m_map, result);
        return result;
    }

}; // class MapHandler

TEST_CASE("decode properties of a feature into map") {
    const auto data = load_test_tile();
    const vtzero::vector_tile tile{data};
    const auto layer = tile.get_layer_by_name("bridge");
    const auto feature = *layer.begin();
    REQUIRE(feature);

    MapHandler handler;
    auto result = feature.decode_attributes(handler);

    REQUIRE(result.size() == 2);
    REQUIRE(result["class"] == "main");
    REQUIRE(result["type"] == "primary");
}

