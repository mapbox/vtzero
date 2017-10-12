
#include <test.hpp>

#include <vtzero/builder.hpp>

#ifdef VTZERO_TEST_WITH_VARIANT
# include <boost/variant.hpp>
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
#endif

#include <map>
#include <string>
#include <unordered_map>

TEST_CASE("property map") {
    vtzero::tile_builder tile;
    vtzero::layer_builder layer_points{tile, "points"};
    {
        vtzero::point_feature_builder feature{layer_points, 1};
        feature.add_points(1);
        feature.set_point(10, 10);
        feature.add_property("foo", "bar");
        feature.add_property("x", "y");
        feature.add_property("abc", "def");
    }

    std::string data = tile.serialize();

    vtzero::vector_tile vt{data};
    REQUIRE(vt.size() == 1);
    auto layer = vt.next_layer();
    REQUIRE(layer.size() == 1);
    auto feature = layer.next_feature();

    REQUIRE(feature.size() == 3);

#ifdef VTZERO_TEST_WITH_VARIANT
    SECTION("std::map") {
        using prop_map_type = std::map<std::string, variant_type>;
        auto map = vtzero::create_properties_map<prop_map_type>(feature);

        REQUIRE(map.size() == 3);
        REQUIRE(boost::get<std::string>(map["foo"]) == "bar");
        REQUIRE(boost::get<std::string>(map["x"]) == "y");
        REQUIRE(boost::get<std::string>(map["abc"]) == "def");
    }
    SECTION("std::unordered_map") {
        using prop_map_type = std::unordered_map<std::string, variant_type>;
        auto map = vtzero::create_properties_map<prop_map_type>(feature);

        REQUIRE(map.size() == 3);
        REQUIRE(boost::get<std::string>(map["foo"]) == "bar");
        REQUIRE(boost::get<std::string>(map["x"]) == "y");
        REQUIRE(boost::get<std::string>(map["abc"]) == "def");
    }
#endif
}

