#ifndef TEST_ATTR_HPP
#define TEST_ATTR_HPP

#include <vtzero/attributes.hpp>
#include <cstdint>

inline uint64_t create_complex_value(const vtzero::detail::complex_value_type type, const uint64_t param) noexcept {
    vtzero_assert_in_noexcept_function((param & 0xf0000000u) == 0u);
    return static_cast<uint64_t>(type) | (param << 4u);
}

inline uint64_t number_list(const uint64_t count) noexcept {
    return create_complex_value(vtzero::detail::complex_value_type::cvt_number_list, count);
}

#endif // TEST_ATTR_HPP
