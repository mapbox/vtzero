
#include <test.hpp>

#include <vtzero/feature.hpp>

TEST_CASE("default constructed feature") {
    vtzero::feature feature{};

    REQUIRE_FALSE(feature.valid());
    REQUIRE_FALSE(feature);
    REQUIRE(feature.id() == 0);
    REQUIRE_FALSE(feature.has_id());
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
    REQUIRE_ASSERT(feature.geometry());
    REQUIRE(feature.empty());
    REQUIRE(feature.num_properties() == 0);
}

