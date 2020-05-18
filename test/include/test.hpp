#ifndef TEST_HPP
#define TEST_HPP

#include <catch.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

// Define vtzero_assert() to throw this error. This allows the tests to
// check that the assert fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define vtzero_assert(x) if (!(x)) { throw assert_error{#x}; }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define vtzero_assert_in_noexcept_function(x) if (!(x)) { got_an_assert = true; }

extern bool got_an_assert;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REQUIRE_ASSERT(x) x; REQUIRE(got_an_assert); got_an_assert = false;

#include <vtzero/output.hpp>

std::string load_test_tile();
std::string load_fixture_tile(const char* env, const std::string& path);

struct counter_attribute_handler {

    int count_ki = 0;
    int count_k = 0;
    int count_v = 0;
    int count_nonscalar = 0;
    std::size_t count_number_list = 0;

    bool key_index(vtzero::index_value /*ki*/, std::size_t /*depth*/) noexcept {
        ++count_ki;
        return true;
    }

    bool attribute_key(vtzero::data_view /*key*/, std::size_t /*depth*/) noexcept {
        ++count_k;
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) noexcept {
        ++count_v;
        return true;
    }

    bool start_list_attribute(std::size_t /*size*/, std::size_t /*depth*/) noexcept {
        ++count_nonscalar;
        ++count_v;
        return true;
    }

    bool end_list_attribute(std::size_t /*depth*/) noexcept {
        --count_nonscalar;
        return true;
    }

    bool start_map_attribute(std::size_t /*size*/, std::size_t /*depth*/) noexcept {
        ++count_nonscalar;
        ++count_v;
        return true;
    }

    bool end_map_attribute(std::size_t /*depth*/) noexcept {
        --count_nonscalar;
        return true;
    }

    bool start_number_list(std::size_t count, vtzero::index_value index, std::size_t /*depth*/) {
        REQUIRE(index.value() == 0);
        count_number_list += count;
        return true;
    }

    std::pair<int, int> result() const {
        REQUIRE(count_nonscalar == 0);
        REQUIRE(count_ki == count_k);
        return std::make_pair(count_k, count_v);
    }

}; // struct counter_attribute_handler

class dump_attribute_handler {

    std::vector<std::string> out;
    bool m_sort = false;

public:

    /**
     * Constructor
     *
     * @param sort Should the attributes be sorted before the result is
     *             returned? Do not use sorting if there are nested attributes
     *             (of type list, number-list, or map). It will not do what
     *             you might expect.
     */
    explicit dump_attribute_handler(bool sort = false) :
        m_sort(sort) {
    }

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        out.emplace_back(key.data(), key.size());
        out.back() += '=';
        return true;
    }

    bool start_list_attribute(std::size_t size, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "list(";
        out.back() += std::to_string(size);
        out.back() += ")[\n";
        return true;
    }

    bool end_list_attribute(std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "]\n";
        return true;
    }

    bool start_map_attribute(std::size_t size, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "map(";
        out.back() += std::to_string(size);
        out.back() += ")[\n";
        return true;
    }

    bool start_number_list(std::size_t size, vtzero::index_value index, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "number-list(";
        out.back() += std::to_string(size);
        out.back() += ',';
        out.back() += std::to_string(index.value());
        out.back() += ")[\n";
        return true;
    }

    bool number_list_null_value(std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "null\n";
        return true;
    }

    bool number_list_value(int64_t value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += std::to_string(value);
        out.back() += '\n';
        return true;
    }

    bool end_number_list(std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "]\n";
        return true;
    }

    bool end_map_attribute(std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "]\n";
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back().append(value.data(), value.size());
        out.back() += '\n';
        return true;
    }

    bool attribute_value(bool value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += (value ? "true\n" : "false\n");
        return true;
    }

    bool attribute_value(vtzero::null_type /*value*/, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "null\n";
        return true;
    }

    bool attribute_value(double value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "double(";
        out.back() += std::to_string(value);
        out.back() += ")\n";
        return true;
    }

    bool attribute_value(float value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "float(";
        out.back() += std::to_string(value);
        out.back() += ")\n";
        return true;
    }

    bool attribute_value(int64_t value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "sint(";
        out.back() += std::to_string(value);
        out.back() += ")\n";
        return true;
    }

    bool attribute_value(uint64_t value, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "uint(";
        out.back() += std::to_string(value);
        out.back() += ")\n";
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) {
        assert(!out.empty());
        out.back() += "ERROR\n";
        return true;
    }

    std::string result() noexcept {
        if (m_sort) {
            std::sort(out.begin(), out.end());
        }
        std::string all;
        for (const auto& s : out) {
            all += s;
        }
        return all;
    }

}; // class dump_attribute_handler

#endif // TEST_HPP
