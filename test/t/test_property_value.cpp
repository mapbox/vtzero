
#include <test.hpp>

#include <vtzero/property_value.hpp>
#include <vtzero/property_value_view.hpp>
#include <vtzero/property_view.hpp>
#include <vtzero/types.hpp>

#ifdef VTZERO_TEST_WITH_VARIANT
# include <boost/variant.hpp>
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
#endif

#include <string>

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

TEST_CASE("default constructed property_value_view") {
    vtzero::property_value_view pvv;
    REQUIRE_FALSE(pvv.valid());
    REQUIRE(pvv.data().data() == nullptr);

    REQUIRE(pvv == vtzero::property_value_view{});
    REQUIRE_FALSE(pvv != vtzero::property_value_view{});
}

TEST_CASE("empty property_value_view") {
    char x[1] = {0};
    vtzero::data_view dv{x, 0};
    vtzero::property_value_view pvv{dv};
    REQUIRE(pvv.valid());
    REQUIRE_THROWS_AS(pvv.type(), const vtzero::format_exception&);
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
    vtzero::property_value v{3.4};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.double_value() == Approx(3.4));

    const auto result = vtzero::apply_visitor(visitor_test_int{}, vv);
    REQUIRE(result == 1);
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

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "42");
}

TEST_CASE("bool value") {
    vtzero::property_value v{true};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.bool_value());

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "1");
}

TEST_CASE("property_value_view equality comparisons") {
    using pvv = vtzero::property_value_view;

    vtzero::property_value t{true};
    vtzero::property_value f{false};
    vtzero::property_value v1{vtzero::int_value_type{1}};
    vtzero::property_value vs{"foo"};

    REQUIRE(pvv{t.data()} == pvv{t.data()});
    REQUIRE_FALSE(pvv{t.data()} != pvv{t.data()});
    REQUIRE_FALSE(pvv{t.data()} == pvv{f.data()});
    REQUIRE_FALSE(pvv{t.data()} == pvv{v1.data()});
    REQUIRE_FALSE(pvv{t.data()} == pvv{vs.data()});
}

TEST_CASE("property_value_view ordering") {
    using pvv = vtzero::property_value_view;

    vtzero::property_value t{true};
    vtzero::property_value f{false};

    REQUIRE_FALSE(pvv{t.data()} <  pvv{f.data()});
    REQUIRE_FALSE(pvv{t.data()} <= pvv{f.data()});
    REQUIRE(pvv{t.data()} >  pvv{f.data()});
    REQUIRE(pvv{t.data()} >= pvv{f.data()});

    vtzero::property_value v1{vtzero::int_value_type{22}};
    vtzero::property_value v2{vtzero::int_value_type{17}};

    REQUIRE_FALSE(pvv{v1.data()} <  pvv{v2.data()});
    REQUIRE_FALSE(pvv{v1.data()} <= pvv{v2.data()});
    REQUIRE(pvv{v1.data()} >  pvv{v2.data()});
    REQUIRE(pvv{v1.data()} >= pvv{v2.data()});

    vtzero::property_value vsf{"foo"};
    vtzero::property_value vsb{"bar"};
    vtzero::property_value vsx{"foobar"};

    REQUIRE_FALSE(pvv{vsf.data()} <  pvv{vsb.data()});
    REQUIRE_FALSE(pvv{vsf.data()} <= pvv{vsb.data()});
    REQUIRE(pvv{vsf.data()} >  pvv{vsb.data()});
    REQUIRE(pvv{vsf.data()} >= pvv{vsb.data()});

    REQUIRE(pvv{vsf.data()} <  pvv{vsx.data()});
    REQUIRE(pvv{vsf.data()} <= pvv{vsx.data()});
    REQUIRE_FALSE(pvv{vsf.data()} >  pvv{vsx.data()});
    REQUIRE_FALSE(pvv{vsf.data()} >= pvv{vsx.data()});
}

TEST_CASE("default constructed property_view") {
    vtzero::property_view pv;
    REQUIRE_FALSE(pv.valid());
    REQUIRE_FALSE(pv);
    REQUIRE(pv.key().data() == nullptr);
    REQUIRE(pv.value().data().data() == nullptr);
}

TEST_CASE("valid property_view") {
    vtzero::data_view k{"key"};
    vtzero::property_value v{"value"};
    vtzero::property_value_view vv{v.data()};

    vtzero::property_view pv{k, vv};
    REQUIRE(pv.key() == "key");
    REQUIRE(pv.value().string_value() == "value");
}

