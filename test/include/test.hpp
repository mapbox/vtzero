#ifndef TEST_HPP
#define TEST_HPP

#include <catch.hpp>

#include <stdexcept>

// Define vtzero_assert() to throw this error. This allows the tests to
// check that the assert fails.
struct assert_error : public std::runtime_error {
    explicit assert_error(const char* what_arg) : std::runtime_error(what_arg) {
    }
};

#define vtzero_assert(x) if (!(x)) { throw assert_error{#x}; }

#define vtzero_assert_in_noexcept_function(x) if (!(x)) { got_an_assert = true; }

extern bool got_an_assert;

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

struct dump_attribute_handler {

    std::string out{};

    bool attribute_key(vtzero::data_view key, std::size_t /*depth*/) {
        out.append(key.data(), key.size());
        out += '=';
        return true;
    }

    bool start_list_attribute(std::size_t size, std::size_t /*depth*/) {
        out += "list(";
        out += std::to_string(size);
        out += ")[\n";
        return true;
    }

    bool end_list_attribute(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool start_map_attribute(std::size_t size, std::size_t /*depth*/) {
        out += "map(";
        out += std::to_string(size);
        out += ")[\n";
        return true;
    }

    bool start_number_list(std::size_t size, vtzero::index_value index, std::size_t /*depth*/) {
        out += "number-list(";
        out += std::to_string(size);
        out += ',';
        out += std::to_string(index.value());
        out += ")[\n";
        return true;
    }

    bool number_list_null_value(std::size_t /*depth*/) {
        out += "null\n";
        return true;
    }

    bool number_list_value(int64_t value, std::size_t /*depth*/) {
        out += std::to_string(value);
        out += '\n';
        return true;
    }

    bool end_number_list(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool end_map_attribute(std::size_t /*depth*/) {
        out += "]\n";
        return true;
    }

    bool attribute_value(vtzero::data_view value, std::size_t /*depth*/) {
        out.append(value.data(), value.size());
        out += '\n';
        return true;
    }

    bool attribute_value(bool value, std::size_t /*depth*/) {
        out += (value ? "true\n" : "false\n");
        return true;
    }

    bool attribute_value(vtzero::null_type /*value*/, std::size_t /*depth*/) {
        out += "null\n";
        return true;
    }

    bool attribute_value(double value, std::size_t /*depth*/) {
        out += "double(";
        out += std::to_string(value);
        out += ")\n";
        return true;
    }

    bool attribute_value(float value, std::size_t /*depth*/) {
        out += "float(";
        out += std::to_string(value);
        out += ")\n";
        return true;
    }

    bool attribute_value(int64_t value, std::size_t /*depth*/) {
        out += "sint(";
        out += std::to_string(value);
        out += ")\n";
        return true;
    }

    bool attribute_value(uint64_t value, std::size_t /*depth*/) {
        out += "uint(";
        out += std::to_string(value);
        out += ")\n";
        return true;
    }

    template <typename T>
    bool attribute_value(T /*value*/, std::size_t /*depth*/) {
        out += "ERROR\n";
        return true;
    }

    std::string result() const noexcept {
        return out;
    }

}; // struct dump_attribute_handler

#endif // TEST_HPP
