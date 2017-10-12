
#include <test.hpp>

#include <vtzero/exception.hpp>

#include <string>

TEST_CASE("exceptions messages for format_exception") {
    vtzero::format_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("exceptions messages for geometry_exception") {
    vtzero::geometry_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("exceptions messages for type_exception") {
    vtzero::type_exception e;
    REQUIRE(std::string{e.what()} == "wrong property value type");
}

TEST_CASE("exceptions messages for version_exception") {
    vtzero::version_exception e{42};
    REQUIRE(std::string{e.what()} == "unknown vector tile version 42");
}

