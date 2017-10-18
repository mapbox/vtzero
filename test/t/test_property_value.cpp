
#include <test.hpp>

#include <vtzero/encoded_property_value.hpp>
#include <vtzero/property.hpp>
#include <vtzero/property_value.hpp>
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

TEST_CASE("default constructed property_value") {
    vtzero::property_value pv;
    REQUIRE_FALSE(pv.valid());
    REQUIRE(pv.data().data() == nullptr);

    REQUIRE(pv == vtzero::property_value{});
    REQUIRE_FALSE(pv != vtzero::property_value{});
}

TEST_CASE("empty property_value") {
    char x[1] = {0};
    vtzero::data_view dv{x, 0};
    vtzero::property_value pv{dv};
    REQUIRE(pv.valid());
    REQUIRE_THROWS_AS(pv.type(), const vtzero::format_exception&);
}

TEST_CASE("string value") {
    vtzero::encoded_property_value epv{"foo"};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.string_value() == "foo");

    visitor_test_void vt;
    vtzero::apply_visitor(vt, pv);
    REQUIRE(vt.x == 2);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 2);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "foo");

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type>(pv);
    REQUIRE(boost::get<std::string>(vari) == "foo");
#endif
}

TEST_CASE("float value") {
    vtzero::encoded_property_value epv{1.2f};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.float_value() == Approx(1.2));

    visitor_test_void vt;
    vtzero::apply_visitor(vt, pv);
    REQUIRE(vt.x == 1);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 1);

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type, std::string>(pv);
    REQUIRE(boost::get<float>(vari) == Approx(1.2));
#endif
}

TEST_CASE("double value") {
    vtzero::encoded_property_value epv{3.4};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.double_value() == Approx(3.4));

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 1);
}

TEST_CASE("int value") {
    vtzero::encoded_property_value epv{vtzero::int_value_type{42}};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.int_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "42");
}

TEST_CASE("uint value") {
    vtzero::encoded_property_value epv{vtzero::uint_value_type{99}};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.uint_value() == 99);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "99");
}

TEST_CASE("sint value") {
    vtzero::encoded_property_value epv{vtzero::sint_value_type{42}};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.sint_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "42");
}

TEST_CASE("bool value") {
    vtzero::encoded_property_value epv{true};
    vtzero::property_value pv{epv.data()};
    REQUIRE(pv.bool_value());

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "1");
}

TEST_CASE("property_value_view equality comparisons") {
    using pv = vtzero::property_value;

    vtzero::encoded_property_value t{true};
    vtzero::encoded_property_value f{false};
    vtzero::encoded_property_value v1{vtzero::int_value_type{1}};
    vtzero::encoded_property_value vs{"foo"};

    REQUIRE(pv{t.data()} == pv{t.data()});
    REQUIRE_FALSE(pv{t.data()} != pv{t.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{f.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{v1.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{vs.data()});
}

TEST_CASE("property_value_view ordering") {
    using pv = vtzero::property_value;

    vtzero::encoded_property_value t{true};
    vtzero::encoded_property_value f{false};

    REQUIRE_FALSE(pv{t.data()} <  pv{f.data()});
    REQUIRE_FALSE(pv{t.data()} <= pv{f.data()});
    REQUIRE(pv{t.data()} >  pv{f.data()});
    REQUIRE(pv{t.data()} >= pv{f.data()});

    vtzero::encoded_property_value v1{vtzero::int_value_type{22}};
    vtzero::encoded_property_value v2{vtzero::int_value_type{17}};

    REQUIRE_FALSE(pv{v1.data()} <  pv{v2.data()});
    REQUIRE_FALSE(pv{v1.data()} <= pv{v2.data()});
    REQUIRE(pv{v1.data()} >  pv{v2.data()});
    REQUIRE(pv{v1.data()} >= pv{v2.data()});

    vtzero::encoded_property_value vsf{"foo"};
    vtzero::encoded_property_value vsb{"bar"};
    vtzero::encoded_property_value vsx{"foobar"};

    REQUIRE_FALSE(pv{vsf.data()} <  pv{vsb.data()});
    REQUIRE_FALSE(pv{vsf.data()} <= pv{vsb.data()});
    REQUIRE(pv{vsf.data()} >  pv{vsb.data()});
    REQUIRE(pv{vsf.data()} >= pv{vsb.data()});

    REQUIRE(pv{vsf.data()} <  pv{vsx.data()});
    REQUIRE(pv{vsf.data()} <= pv{vsx.data()});
    REQUIRE_FALSE(pv{vsf.data()} >  pv{vsx.data()});
    REQUIRE_FALSE(pv{vsf.data()} >= pv{vsx.data()});
}

TEST_CASE("default constructed property_view") {
    vtzero::property p;
    REQUIRE_FALSE(p.valid());
    REQUIRE_FALSE(p);
    REQUIRE(p.key().data() == nullptr);
    REQUIRE(p.value().data().data() == nullptr);
}

TEST_CASE("valid property_view") {
    vtzero::data_view k{"key"};
    vtzero::encoded_property_value epv{"value"};
    vtzero::property_value pv{epv.data()};

    vtzero::property p{k, pv};
    REQUIRE(p.key() == "key");
    REQUIRE(p.value().string_value() == "value");
}

