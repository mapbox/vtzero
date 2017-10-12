
#include <test.hpp>

#include <vtzero/vector_tile.hpp>
#include <vtzero/layer.hpp>

TEST_CASE("read a layer") {
    const auto data = load_test_tile();
    vtzero::vector_tile tile{data};

    auto layer = tile.get_layer_by_name("bridge");
    REQUIRE(layer.version() == 1);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.name() == "bridge");
    REQUIRE(layer.num_features() == 2);
}

