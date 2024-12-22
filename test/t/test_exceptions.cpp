
#include <test.hpp>

#include <vtzero/exception.hpp>

#include <string>

TEST_CASE("construct format_exception with const char*") {
    const vtzero::format_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct format_exception with const std::string") {
    const vtzero::format_exception e{std::string{"broken"}};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct geometry_exception with const char*") {
    const vtzero::geometry_exception e{"broken"};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct geometry_exception with std::string") {
    const vtzero::geometry_exception e{std::string{"broken"}};
    REQUIRE(std::string{e.what()} == "broken");
}

TEST_CASE("construct type_exception") {
    const vtzero::type_exception e;
    REQUIRE(std::string{e.what()} == "wrong property value type");
}

TEST_CASE("construct version_exception") {
    const vtzero::version_exception e{42};
    REQUIRE(std::string{e.what()} == "unknown vector tile version: 42");
}

TEST_CASE("construct out_of_range_exception") {
    const vtzero::out_of_range_exception e{99};
    REQUIRE(std::string{e.what()} == "index out of range: 99");
}

