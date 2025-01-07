
#include <test.hpp>

#include <vtzero/encoded_property_value.hpp>
#include <vtzero/property.hpp>
#include <vtzero/property_value.hpp>
#include <vtzero/types.hpp>

#ifdef VTZERO_TEST_WITH_VARIANT
# include <boost/variant.hpp>
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;

struct variant_mapping : vtzero::property_value_mapping {
    using float_type = int64_t;
    using double_type = int64_t;
};

#endif

#include <cstdint>
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

struct string_conv {

    std::string s;

    template <typename T>
    explicit string_conv(T value) :
        s(std::to_string(value)) {
    }

    explicit operator std::string() const {
        return s;
    }

};

struct string_mapping : vtzero::property_value_mapping {
    using string_type = std::string;
    using float_type  = string_conv;
    using double_type = string_conv;
    using int_type    = string_conv;
    using uint_type   = string_conv;
    using bool_type   = string_conv;
};

TEST_CASE("default constructed property_value") {
    const vtzero::property_value pv;
    REQUIRE_FALSE(pv.valid());
    REQUIRE(pv.data().data() == nullptr);

    REQUIRE(pv == vtzero::property_value{});
    REQUIRE_FALSE(pv != vtzero::property_value{});
}

TEST_CASE("empty property_value") {
    const char x = 0;
    const vtzero::data_view dv{&x, 0};
    const vtzero::property_value pv{dv};
    REQUIRE(pv.valid());
    REQUIRE_THROWS_AS(pv.type(), vtzero::format_exception);
}

TEST_CASE("string value") {
    const vtzero::encoded_property_value epv{"foo"};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.string_value() == "foo");

    visitor_test_void vt;
    vtzero::apply_visitor(vt, pv);
    REQUIRE(vt.x == 2);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 2);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "foo");

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "foo");

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type>(pv);
    REQUIRE(boost::get<std::string>(vari) == "foo");
    const auto conv = vtzero::convert_property_value<variant_type, variant_mapping>(pv);
    REQUIRE(boost::get<std::string>(conv) == "foo");
#endif
}

TEST_CASE("float value") {
    const vtzero::encoded_property_value epv{1.2F};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.float_value() == Approx(1.2));

    visitor_test_void vt;
    vtzero::apply_visitor(vt, pv);
    REQUIRE(vt.x == 1);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 1);

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "1.200000");

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type>(pv);
    REQUIRE(boost::get<float>(vari) == Approx(1.2));
    const auto conv = vtzero::convert_property_value<variant_type, variant_mapping>(pv);
    REQUIRE(boost::get<int64_t>(conv) == 1);
#endif
}

TEST_CASE("double value") {
    const vtzero::encoded_property_value epv{3.4};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.double_value() == Approx(3.4));

    const auto result = vtzero::apply_visitor(visitor_test_int{}, pv);
    REQUIRE(result == 1);

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "3.400000");
}

TEST_CASE("int value") {
    const vtzero::encoded_property_value epv{vtzero::int_value_type{42}};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.int_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "42");

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "42");
}

TEST_CASE("uint value") {
    const vtzero::encoded_property_value epv{vtzero::uint_value_type{99}};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.uint_value() == 99);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "99");

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "99");
}

TEST_CASE("sint value") {
    const vtzero::encoded_property_value epv{vtzero::sint_value_type{42}};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.sint_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "42");

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "42");
}

TEST_CASE("bool value") {
    const vtzero::encoded_property_value epv{true};
    const vtzero::property_value pv{epv.data()};
    REQUIRE(pv.bool_value());

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, pv);
    REQUIRE(str == "1");

    const std::string cs = vtzero::convert_property_value<std::string, string_mapping>(pv);
    REQUIRE(cs == "1");
}

TEST_CASE("property and property_value equality comparisons") {
    const vtzero::encoded_property_value t{true};
    const vtzero::encoded_property_value f{false};
    const vtzero::encoded_property_value v1{vtzero::int_value_type{1}};
    const vtzero::encoded_property_value vs{"foo"};

    REQUIRE(t == t);
    REQUIRE_FALSE(t != t);
    REQUIRE_FALSE(t == f);
    REQUIRE_FALSE(t == v1);
    REQUIRE_FALSE(t == vs);

    using pv = vtzero::property_value;
    REQUIRE(pv{t.data()} == pv{t.data()});
    REQUIRE_FALSE(pv{t.data()} != pv{t.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{f.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{v1.data()});
    REQUIRE_FALSE(pv{t.data()} == pv{vs.data()});
}

TEST_CASE("property and property_value ordering") {
    using pv = vtzero::property_value;

    const vtzero::encoded_property_value t{true};
    const vtzero::encoded_property_value f{false};

    REQUIRE_FALSE(t <  f);
    REQUIRE_FALSE(t <= f);
    REQUIRE(t >  f);
    REQUIRE(t >= f);

    REQUIRE_FALSE(pv{t.data()} <  pv{f.data()});
    REQUIRE_FALSE(pv{t.data()} <= pv{f.data()});
    REQUIRE(pv{t.data()} >  pv{f.data()});
    REQUIRE(pv{t.data()} >= pv{f.data()});

    const vtzero::encoded_property_value v1{vtzero::int_value_type{22}};
    const vtzero::encoded_property_value v2{vtzero::int_value_type{17}};

    REQUIRE_FALSE(v1 <  v2);
    REQUIRE_FALSE(v1 <= v2);
    REQUIRE(v1 >  v2);
    REQUIRE(v1 >= v2);

    REQUIRE_FALSE(pv{v1.data()} <  pv{v2.data()});
    REQUIRE_FALSE(pv{v1.data()} <= pv{v2.data()});
    REQUIRE(pv{v1.data()} >  pv{v2.data()});
    REQUIRE(pv{v1.data()} >= pv{v2.data()});

    const vtzero::encoded_property_value vsf{"foo"};
    const vtzero::encoded_property_value vsb{"bar"};
    const vtzero::encoded_property_value vsx{"foobar"};

    REQUIRE_FALSE(vsf <  vsb);
    REQUIRE_FALSE(vsf <= vsb);
    REQUIRE(vsf >  vsb);
    REQUIRE(vsf >= vsb);

    REQUIRE_FALSE(pv{vsf.data()} <  pv{vsb.data()});
    REQUIRE_FALSE(pv{vsf.data()} <= pv{vsb.data()});
    REQUIRE(pv{vsf.data()} >  pv{vsb.data()});
    REQUIRE(pv{vsf.data()} >= pv{vsb.data()});

    REQUIRE(vsf <  vsx);
    REQUIRE(vsf <= vsx);
    REQUIRE_FALSE(vsf >  vsx);
    REQUIRE_FALSE(vsf >= vsx);

    REQUIRE(pv{vsf.data()} <  pv{vsx.data()});
    REQUIRE(pv{vsf.data()} <= pv{vsx.data()});
    REQUIRE_FALSE(pv{vsf.data()} >  pv{vsx.data()});
    REQUIRE_FALSE(pv{vsf.data()} >= pv{vsx.data()});
}

TEST_CASE("default constructed property") {
    const vtzero::property p;
    REQUIRE_FALSE(p.valid());
    REQUIRE_FALSE(p);
    REQUIRE(p.key().data() == nullptr);
    REQUIRE(p.value().data().data() == nullptr);
}

TEST_CASE("valid property") {
    const vtzero::data_view k{"key"};
    const vtzero::encoded_property_value epv{"value"};
    const vtzero::property_value pv{epv.data()};

    const vtzero::property p{k, pv};
    REQUIRE(p.key() == "key");
    REQUIRE(p.value().string_value() == "value");
}

TEST_CASE("create encoded property values from different string types") {
    const std::string v{"value"};

    const vtzero::encoded_property_value epv1{vtzero::string_value_type{"value"}};
    const vtzero::encoded_property_value epv2{"value"};
    const vtzero::encoded_property_value epv3{v};
    const vtzero::encoded_property_value epv4{vtzero::data_view{v}};
    const vtzero::encoded_property_value epv5{"valuexxxxxxxxx", 5};

    REQUIRE(epv1 == epv2);
    REQUIRE(epv1 == epv3);
    REQUIRE(epv1 == epv4);
    REQUIRE(epv1 == epv5);
}

TEST_CASE("create encoded property values from different floating point types") {
    const vtzero::encoded_property_value f1{vtzero::float_value_type{3.2F}};
    const vtzero::encoded_property_value f2{3.2F};
    const vtzero::encoded_property_value d1{vtzero::double_value_type{3.2}};
    const vtzero::encoded_property_value d2{3.2};

    REQUIRE(f1 == f2);
    REQUIRE(d1 == d2);

    const vtzero::property_value pvf{f1.data()};
    const vtzero::property_value pvd{d1.data()};

    REQUIRE(pvf.float_value() == Approx(pvd.double_value()));
}

TEST_CASE("create encoded property values from different integer types") {
    const vtzero::encoded_property_value i1{vtzero::int_value_type{7}};
    const vtzero::encoded_property_value i2{static_cast<int64_t>(7)};
    const vtzero::encoded_property_value i3{static_cast<int32_t>(7)};
    const vtzero::encoded_property_value i4{static_cast<int16_t>(7)};
    const vtzero::encoded_property_value u1{vtzero::uint_value_type{7}};
    const vtzero::encoded_property_value u2{static_cast<uint64_t>(7)};
    const vtzero::encoded_property_value u3{static_cast<uint32_t>(7)};
    const vtzero::encoded_property_value u4{static_cast<uint16_t>(7)};
    const vtzero::encoded_property_value s1{vtzero::sint_value_type{7}};

    REQUIRE(i1 == i2);
    REQUIRE(i1 == i3);
    REQUIRE(i1 == i4);
    REQUIRE(u1 == u2);
    REQUIRE(u1 == u3);
    REQUIRE(u1 == u4);

    REQUIRE_FALSE(i1 == u1);
    REQUIRE_FALSE(i1 == s1);
    REQUIRE_FALSE(u1 == s1);

    REQUIRE(i1.hash() == i2.hash());
    REQUIRE(u1.hash() == u2.hash());

    const vtzero::property_value pvi{i1.data()};
    const vtzero::property_value pvu{u1.data()};
    const vtzero::property_value pvs{s1.data()};

    REQUIRE(pvi.int_value() == static_cast<int64_t>(pvu.uint_value()));
    REQUIRE(pvi.int_value() == pvs.sint_value());
}

TEST_CASE("create encoded property values from different bool types") {
    const vtzero::encoded_property_value b1{vtzero::bool_value_type{true}};
    const vtzero::encoded_property_value b2{true};

    REQUIRE(b1 == b2);
    REQUIRE(b1.hash() == b2.hash());
}

TEST_CASE("property equality comparison operator") {
    const std::string k = "key";

    const vtzero::encoded_property_value epv1{"value"};
    const vtzero::encoded_property_value epv2{"another value"};
    const vtzero::property_value pv1{epv1.data()};
    const vtzero::property_value pv2{epv2.data()};

    const vtzero::property p1{k, pv1};
    const vtzero::property p2{k, pv1};
    const vtzero::property p3{k, pv2};
    REQUIRE(p1 == p2);
    REQUIRE_FALSE(p1 == p3);
}

TEST_CASE("property inequality comparison operator") {
    const std::string k1 = "key";
    const std::string k2 = "another_key";

    const vtzero::encoded_property_value epv1{"value"};
    const vtzero::encoded_property_value epv2{"another value"};
    const vtzero::property_value pv1{epv1.data()};
    const vtzero::property_value pv2{epv2.data()};

    const vtzero::property p1{k1, pv1};
    const vtzero::property p2{k1, pv1};
    const vtzero::property p3{k1, pv2};
    const vtzero::property p4{k2, pv2};
    REQUIRE_FALSE(p1 != p2);
    REQUIRE(p1 != p3);
    REQUIRE(p3 != p4);
}
