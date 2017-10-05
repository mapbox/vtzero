
#include <vtzero/property_value.hpp>
#include <vtzero/property_value_view.hpp>
#include <vtzero/types.hpp>

#include <catch.hpp>

struct visitor_test_void {

    int x = 0;

    template <typename T>
    void operator()(T /*value*/) {
        x = 1;
    }

    void operator()(vtzero::data_view /*value*/) {
        x = 2;
    }

};

struct visitor_test_int {

    template <typename T>
    int operator()(T /*value*/) {
        return 1;
    }

    int operator()(vtzero::data_view /*value*/) {
        return 2;
    }

};

struct visitor_test_to_string {

    template <typename T>
    std::string operator()(T value) {
        return std::to_string(value);
    }

    std::string operator()(vtzero::data_view value) {
        return std::string{value.data(), value.size()};
    }

};

#ifdef VTZERO_TEST_WITH_VARIANT
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
#endif

TEST_CASE("empty property_value_view") {
    char x[1] = {0};
    vtzero::data_view dv{x, 1};
    vtzero::property_value_view pvv{dv};
    REQUIRE_THROWS(pvv.type());
}

TEST_CASE("string value") {
    vtzero::property_value v{"foo"};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.string_value() == "foo");

    visitor_test_void vt;
    vtzero::apply_visitor(vt, vv);
    REQUIRE(vt.x == 2);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, vv);
    REQUIRE(result == 2);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "foo");

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type>(vv);
    REQUIRE(boost::get<std::string>(vari) == "foo");
#endif
}

TEST_CASE("float value") {
    vtzero::property_value v{1.2f};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.float_value() == Approx(1.2));

    visitor_test_void vt;
    vtzero::apply_visitor(vt, vv);
    REQUIRE(vt.x == 1);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, vv);
    REQUIRE(result == 1);

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type, std::string>(vv);
    REQUIRE(boost::get<float>(vari) == Approx(1.2));
#endif
}

TEST_CASE("double value") {
    vtzero::property_value v{1.2};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.double_value() == Approx(1.2));
}

TEST_CASE("int value") {
    vtzero::property_value v{vtzero::int_value_type{42}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.int_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "42");
}

TEST_CASE("uint value") {
    vtzero::property_value v{vtzero::uint_value_type{99}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.uint_value() == 99);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "99");
}

TEST_CASE("sint value") {
    vtzero::property_value v{vtzero::sint_value_type{42}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.sint_value() == 42);
}

TEST_CASE("bool value") {
    vtzero::property_value v{true};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.bool_value());
}

