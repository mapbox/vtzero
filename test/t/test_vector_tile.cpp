
#include <test.hpp>

#include <vtzero/vector_tile.hpp>

#include <string>
#include <vector>

static_assert(std::is_nothrow_move_constructible<vtzero::vector_tile>::value, "vector_tile is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<vtzero::vector_tile>::value, "vector_tile is not nothrow move assignable");

TEST_CASE("open a vector tile with string") {
    const std::string buffer{load_test_tile()};
    REQUIRE(vtzero::is_vector_tile(buffer));
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("open a vector tile with data_view") {
    const std::string buffer{load_test_tile()};
    const vtzero::data_view dv{buffer};
    const vtzero::vector_tile tile{dv};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("open a vector tile with pointer and size") {
    const std::string buffer{load_test_tile()};
    const vtzero::vector_tile tile{buffer.data(), buffer.size()};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 12);
}

TEST_CASE("get layer by index") {
    const std::string buffer{load_test_tile()};
    const vtzero::vector_tile tile{buffer};

    auto layer = tile.get_layer(0);
    REQUIRE(layer);
    REQUIRE(layer.name() == "landuse");

    layer = tile.get_layer(1);
    REQUIRE(layer);
    REQUIRE(layer.name() == "waterway");

    layer = tile.get_layer(11);
    REQUIRE(layer);
    REQUIRE(layer.name() == "waterway_label");

    layer = tile.get_layer(12);
    REQUIRE_FALSE(layer);
}

TEST_CASE("get layer by name") {
    const std::string buffer{load_test_tile()};
    const vtzero::vector_tile tile{buffer};

    auto layer = tile.get_layer_by_name("landuse");
    REQUIRE(layer);
    REQUIRE(layer.name() == "landuse");

    layer = tile.get_layer_by_name(std::string{"road"});
    REQUIRE(layer);
    REQUIRE(layer.name() == "road");

    const vtzero::data_view name{"poi_label"};
    layer = tile.get_layer_by_name(name);
    REQUIRE(layer);
    REQUIRE(layer.name() == "poi_label");

    layer = tile.get_layer_by_name("unknown");
    REQUIRE_FALSE(layer);
}

TEST_CASE("iterate over layers") {
    const std::string buffer{load_test_tile()};
    const vtzero::vector_tile tile{buffer};

    std::vector<std::string> names;

    for (auto layer : tile) {
        names.emplace_back(layer.name());
    }

    REQUIRE(names.size() == 12);

    static std::vector<std::string> expected = {
        "landuse", "waterway", "water", "barrier_line", "building", "road",
        "bridge", "place_label", "water_label", "poi_label", "road_label",
        "waterway_label"
    };

    REQUIRE(names == expected);

    int num = 0;
    for (auto layer : tile) {
        ++num;
    }
    REQUIRE(num == 12);
}

TEST_CASE("iterate over some of the layers") {
    const std::string buffer{load_test_tile()};
    const vtzero::vector_tile tile{buffer};

    int num_layers = 0;

    for (auto layer : tile) {
        ++num_layers;
        if (layer.name() == "water") {
            break;
        }
    }

    REQUIRE(num_layers == 3);
}

