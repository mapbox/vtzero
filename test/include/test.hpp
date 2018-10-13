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

struct mypoint {
    int64_t p1;
    int64_t p2;

    mypoint() = default;
    mypoint(int64_t x, int64_t y) :
        p1(x),
        p2(y) {
   }
};

inline bool operator==(const mypoint lhs, const mypoint rhs) noexcept {
    return lhs.p1 == rhs.p1 && lhs.p2 == rhs.p2;
}

inline bool operator!=(const mypoint lhs, const mypoint rhs) noexcept {
    return !(lhs == rhs);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const mypoint p) {
    return out << '(' << p.p1 << ',' << p.p2 << ')';
}

inline vtzero::point create_vtzero_point(mypoint p) noexcept {
    return {static_cast<int32_t>(p.p1),
            static_cast<int32_t>(p.p2)};
}

struct mypoint_3d {
    int64_t x = 0;
    int64_t y = 0;
    int64_t elev = 0;

    mypoint_3d(int64_t x_, int64_t y_, int64_t elev_) :
        x(x_),
        y(y_),
        elev(elev_) {
    }
};

inline constexpr bool operator==(const mypoint_3d& a, const mypoint_3d& b) noexcept {
    return a.x == b.x && a.y == b.y && a.elev == b.elev;
}

inline constexpr bool operator!=(const mypoint_3d& a, const mypoint_3d& b) noexcept {
    return !(a == b);
}

template <typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& out, const mypoint_3d& p) {
    return out << '(' << p.x << ',' << p.y << ',' << p.elev << ')';
}

struct AttributeCountHandler {

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

    bool start_number_list(std::size_t count, vtzero::index_value index, std::size_t /*depth*/) noexcept {
        REQUIRE(index.value() == 0);
        count_number_list += count;
        return true;
    }

    std::pair<int, int> result() const {
        REQUIRE(count_nonscalar == 0);
        REQUIRE(count_ki == count_k);
        return std::make_pair(count_k, count_v);
    }

}; // class AttributeCountHandler

struct AttributeDumpHandler {

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

    template <typename T>
    bool attribute_value(T value, std::size_t /*depth*/) {
        out += std::to_string(value);
        out += '\n';
        return true;
    }

    std::string result() const noexcept {
        return out;
    }

}; // class AttributeDumpHandler

#endif // TEST_HPP
