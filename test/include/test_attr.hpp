#ifndef TEST_ATTR_HPP
#define TEST_ATTR_HPP

#include <vtzero/detail/attributes.hpp>

#include <cstdint>

inline uint64_t number_list(const uint64_t count) noexcept {
    return vtzero::detail::create_complex_value(vtzero::detail::complex_value_type::cvt_number_list, count);
}

#endif // TEST_ATTR_HPP
