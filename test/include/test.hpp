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

#endif // TEST_HPP
