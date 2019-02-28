
#ifdef VTZERO_TEST_WITH_VARIANT

#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/feature.hpp>
#include <vtzero/layer.hpp>
#include <vtzero/types.hpp>
#include <vtzero/vector_tile.hpp>

#include <boost/variant.hpp>

#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

using variant_type = boost::make_recursive_variant<
        std::string,
        float,
        double,
        uint64_t,
        int64_t,
        bool,
        vtzero::null_type,
        std::vector<boost::recursive_variant_>,
        std::map<std::string, boost::recursive_variant_>
    >::type;

template <typename TVariant, typename TList = typename std::vector<TVariant>, typename TMap = typename std::map<std::string, TVariant>>
class VariantConverter {

    using list_type = TList;
    using map_type = TMap;

    using rec_type = boost::variant<list_type*, map_type*>;

    std::stack<rec_type> m_stack;
    map_type m_data;
    vtzero::data_view m_key;

    template <typename T>
    void value_impl(T&& value) {
        assert(!m_stack.empty());
        auto top = m_stack.top();
        if (top.which() == 0) {
            boost::get<list_type*>(top)->emplace_back(std::forward<T>(value));
        } else {
            boost::get<map_type*>(top)->emplace(std::string(m_key), std::forward<T>(value));
        }
    }

public:

    VariantConverter() {
        m_stack.emplace(&m_data);
    }

    void attribute_key(vtzero::data_view key, std::size_t /*depth*/) noexcept {
        m_key = key;
    }

    void start_list_attribute(std::size_t size, std::size_t /*depth*/) {
        assert(!m_stack.empty());
        auto top = m_stack.top();
        if (top.which() == 0) {
            list_type* t = boost::get<list_type*>(top);
            t->emplace_back(list_type{});
            auto& list = boost::get<list_type>(t->back());
            list.reserve(size);
            m_stack.emplace(&list);
        } else {
            auto p = boost::get<map_type*>(top)->emplace(std::string(m_key), list_type{});
            auto& list = boost::get<list_type>(p.first->second);
            list.reserve(size);
            m_stack.emplace(&list);
        }
    }

    void end_list_attribute(std::size_t /*depth*/) {
        assert(!m_stack.empty());
        m_stack.pop();
    }

    void start_map_attribute(std::size_t /*size*/, std::size_t /*depth*/) {
        assert(!m_stack.empty());
        auto top = m_stack.top();
        if (top.which() == 0) {
            list_type* t = boost::get<list_type*>(top);
            t->emplace_back(map_type{});
            auto& map = boost::get<map_type>(t->back());
            m_stack.emplace(&map);
        } else {
            auto p = boost::get<map_type*>(top)->emplace(std::string(m_key), map_type{});
            auto& map = boost::get<map_type>(p.first->second);
            m_stack.emplace(&map);
        }
    }

    void end_map_attribute(std::size_t /*depth*/) {
        assert(!m_stack.empty());
        m_stack.pop();
    }

    void attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        value_impl(std::string(value));
    }

    template <typename T>
    void attribute_value(T value, std::size_t /*depth*/) {
        value_impl(std::forward<T>(value));
    }

    map_type result() {
        assert(m_stack.size() == 1);
        m_stack.pop();
        map_type data;
        using std::swap;
        swap(data, m_data);
        m_stack.emplace(&m_data);
        return data;
    }

}; // class VariantConverter

TEST_CASE("build feature with attributes explicitly and read it again") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.add_scalar_attribute("some_int", 111u);
        fbuilder.attribute_key("list");
        fbuilder.start_list_attribute(8);
        fbuilder.attribute_value(vtzero::data_view{"foo"}); // 1
        fbuilder.attribute_value(17u); // 2
        fbuilder.attribute_value(-22); // 3
        fbuilder.attribute_value(true); // 4
        fbuilder.attribute_value(false); // 5
        fbuilder.attribute_value(vtzero::null_type{}); // 6
        fbuilder.attribute_value("bar"); // 7
        fbuilder.attribute_value(std::string{"baz"}); // 8
        fbuilder.attribute_key("another_int");
        fbuilder.attribute_value(222u);
        fbuilder.attribute_key("map");
        fbuilder.start_map_attribute(2);
        fbuilder.add_scalar_attribute("x", 3); // 1
        fbuilder.add_scalar_attribute("y", 5); // 2
        fbuilder.add_scalar_attribute("a_different_int", 333u);
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    VariantConverter<variant_type> converter;
    const auto result = feature.decode_attributes(converter);

    REQUIRE(boost::get<uint64_t>(result.find("some_int")->second) == 111u);
    REQUIRE(boost::get<uint64_t>(result.find("a_different_int")->second) == 333u);

    const variant_type& lv = result.find("list")->second;
    const auto& vec = boost::get<std::vector<variant_type>>(lv);
    REQUIRE(vec.size() == 8);

    REQUIRE(boost::get<std::string>(vec[0]) == "foo");
    REQUIRE(boost::get<uint64_t>(vec[1]) == 17u);
    REQUIRE(boost::get<int64_t>(vec[2]) == -22);
    REQUIRE(boost::get<bool>(vec[3]));
    REQUIRE_FALSE(boost::get<bool>(vec[4]));

    const variant_type& mv = result.find("map")->second;
    const auto& map = boost::get<std::map<std::string, variant_type>>(mv);
    REQUIRE(map.size() == 2);

    REQUIRE(boost::get<int64_t>(map.find("x")->second) == 3);
    REQUIRE(boost::get<int64_t>(map.find("y")->second) == 5);
}

template <typename TBuilder, typename TVariant>
class attributes_builder_visitor : public boost::static_visitor<> {

    TBuilder& m_builder;

public:

    explicit attributes_builder_visitor(TBuilder& builder) :
        m_builder(builder) {
    }

    void operator()(const std::vector<TVariant>& list) const {
        m_builder.start_list_attribute(list.size());
        attributes_builder_visitor<TBuilder, variant_type> visitor{m_builder};
        for (const auto& member : list) {
            boost::apply_visitor(visitor, member);
        }
    }

    void operator()(const std::map<std::string, TVariant>& map) const {
        m_builder.start_map_attribute(map.size());
        attributes_builder_visitor<TBuilder, variant_type> visitor{m_builder};
        for (const auto& member : map) {
            m_builder.attribute_key(member.first);
            boost::apply_visitor(visitor, member.second);
        }
    }

    template <typename T>
    void operator()(T value) const {
        m_builder.attribute_value(value);
    }

}; // class attributes_builder_visitor

template <typename TBuilder, typename TMap>
static void set_attributes(TBuilder& builder, const TMap& map) {
    attributes_builder_visitor<TBuilder, variant_type> visitor{builder};
    for (const auto& member : map) {
        builder.attribute_key(member.first);
        boost::apply_visitor(visitor, member.second);
    }
}

template <typename... TArgs>
variant_type make_list(TArgs&&... args) {
    return variant_type{std::vector<variant_type>{std::forward<TArgs>(args)...}};
}

template <typename... TArgs>
variant_type make_map(TArgs&&... args) {
    return variant_type{std::map<std::string, variant_type>{std::forward<TArgs>(args)...}};
}

TEST_CASE("build feature with attributes from variant and read it again") {
    std::map<std::string, variant_type> test_data{
        {"some_int", static_cast<uint64_t>(111u)},
        {"list", make_list(
            std::string{"foo"},
            static_cast<uint64_t>(17u),
            static_cast<int64_t>(-22),
            true,
            false,
            vtzero::null_type{},
            std::string{"bar"},
            std::string{"baz"}
        )},
        {"another_int", static_cast<uint64_t>(222u)},
        {"map", make_map(
            std::make_pair("x", static_cast<int64_t>(3)),
            std::make_pair("y", static_cast<int64_t>(5))
        )},
        {"empty_list", make_list()},
        {"empty_map", make_map()},
        {"one_list", make_list(
            true
        )},
        {"rec_list", make_list(
            make_list(true, false, std::string{"abc"}),
            make_map(
                std::make_pair("true", true),
                std::make_pair("false", false)
            )
        )},
        {"a_different_int", static_cast<uint64_t>(333u)}
    };

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        set_attributes(fbuilder, test_data);
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    // =====================================================

    const vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer);
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature);
    REQUIRE(feature.integer_id() == 1);

    VariantConverter<variant_type> converter;
    const auto result = feature.decode_attributes(converter);

    REQUIRE(boost::get<uint64_t>(result.find("some_int")->second) == 111u);
    REQUIRE(boost::get<uint64_t>(result.find("a_different_int")->second) == 333u);

    {
        const variant_type& lv = result.find("list")->second;
        const auto& vec = boost::get<std::vector<variant_type>>(lv);
        REQUIRE(vec.size() == 8);

        REQUIRE(boost::get<std::string>(vec[0]) == "foo");
        REQUIRE(boost::get<uint64_t>(vec[1]) == 17u);
        REQUIRE(boost::get<int64_t>(vec[2]) == -22);
        REQUIRE(boost::get<bool>(vec[3]));
        REQUIRE_FALSE(boost::get<bool>(vec[4]));
    }

    {
        const variant_type& lv = result.find("empty_list")->second;
        const auto& vec = boost::get<std::vector<variant_type>>(lv);
        REQUIRE(vec.empty());
    }

    {
        const variant_type& lv = result.find("one_list")->second;
        const auto& vec = boost::get<std::vector<variant_type>>(lv);
        REQUIRE(vec.size() == 1);
        REQUIRE(boost::get<bool>(vec[0]));
    }

    {
        const variant_type& lv = result.find("rec_list")->second;
        const auto& vec = boost::get<std::vector<variant_type>>(lv);
        REQUIRE(vec.size() == 2);

        const auto& vec2 = boost::get<std::vector<variant_type>>(vec[0]);
        REQUIRE(vec2.size() == 3);
        REQUIRE(boost::get<bool>(vec2[0]));
        REQUIRE_FALSE(boost::get<bool>(vec2[1]));
        REQUIRE(boost::get<std::string>(vec2[2]) == "abc");

        const auto& map = boost::get<std::map<std::string, variant_type>>(vec[1]);
        REQUIRE(map.size() == 2);
        REQUIRE(boost::get<bool>(map.find("true")->second));
        REQUIRE_FALSE(boost::get<bool>(map.find("false")->second));
    }

    {
        const variant_type& mv = result.find("map")->second;
        const auto& map = boost::get<std::map<std::string, variant_type>>(mv);
        REQUIRE(map.size() == 2);

        REQUIRE(boost::get<int64_t>(map.find("x")->second) == 3);
        REQUIRE(boost::get<int64_t>(map.find("y")->second) == 5);
    }

    {
        const variant_type& mv = result.find("empty_map")->second;
        const auto& map = boost::get<std::map<std::string, variant_type>>(mv);
        REQUIRE(map.empty());
    }

    // =====================================================

    vtzero::tile_builder tbuilder2;
    vtzero::layer_builder lbuilder2{tbuilder2, "test", 3};
    {
        vtzero::point_feature_builder<2> fbuilder{lbuilder2};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(vtzero::point_2d{10, 20});
        fbuilder.copy_attributes(feature);
        fbuilder.commit();
    }

    const std::string data2 = tbuilder.serialize();
    REQUIRE(data == data2);
}

#endif

