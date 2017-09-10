#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <vtzero/version.hpp>

TEST_CASE("test version")
{
    REQUIRE(VTZERO_VERSION_STRING == std::string("1.0.0"));
}