#ifndef VTZERO_DETAIL_ATTRIBUTES_HPP
#define VTZERO_DETAIL_ATTRIBUTES_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file attributes.hpp
 *
 * @brief Contains types and functions related to attributes.
 */

#include "../exception.hpp"
#include "../point.hpp"
#include "../types.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace vtzero {

    namespace detail {

        // attribute value types
        enum class structured_value_type {
            cvt_string      =  0,
            cvt_float       =  1,
            cvt_double      =  2,
            cvt_uint        =  3,
            cvt_sint        =  4,
            cvt_inline_uint =  5,
            cvt_inline_sint =  6,
            cvt_bool        =  7,
            cvt_null        =  7,
            cvt_list        =  8,
            cvt_map         =  9,
            cvt_number_list = 10,
            max             = 10
        };

        inline uint64_t create_structured_value(const vtzero::detail::structured_value_type type, const uint64_t param) noexcept {
            vtzero_assert_in_noexcept_function((param & 0xf0000000U) == 0U);
            return static_cast<uint64_t>(type) | (param << 4U);
        }

// @cond internal
        // These macros are used to define "wrapper" functions for calling
        // member functions on a handler class. If the class doesn't define
        // some member function a dummy is called that always returns true.
        // If the class defines the function, it is called.

        // Call a function without parameters
#define DEF_CALL_WRAPPER0(func) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func())>::value, int>::type = 0> \
        bool call_##func(THandler&& handler) { \
            return std::forward<THandler>(handler).func(); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func())>::value, int>::type = 0> \
        bool call_##func(THandler&& handler) { \
            std::forward<THandler>(handler).func(); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        }

        // Call a function with one parameter
#define DEF_CALL_WRAPPER1(func, ptype) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<ptype>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype param) { \
            return std::forward<THandler>(handler).func(param); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<ptype>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype param) { \
            std::forward<THandler>(handler).func(param); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        }

        // Call a function with two parameters
#define DEF_CALL_WRAPPER2(func, ptype1, ptype2) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<ptype1>(), std::declval<ptype2>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype1 param1, ptype2 param2) { \
            return std::forward<THandler>(handler).func(param1, param2); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<ptype1>(), std::declval<ptype2>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype1 param1, ptype2 param2) { \
            std::forward<THandler>(handler).func(param1, param2); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        }

        /// Call a function with three parameters
#define DEF_CALL_WRAPPER3(func, ptype1, ptype2, ptype3) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<ptype1>(), std::declval<ptype2>(), std::declval<ptype3>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype1 param1, ptype2 param2, ptype3 param3) { \
            return std::forward<THandler>(handler).func(param1, param2, param3); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<ptype1>(), std::declval<ptype2>(), std::declval<ptype3>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype1 param1, ptype2 param2, ptype3 param3) { \
            std::forward<THandler>(handler).func(param1, param2, param3); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        }

        DEF_CALL_WRAPPER2(key_index, index_value, std::size_t)
        DEF_CALL_WRAPPER2(value_index, index_value, std::size_t)
        DEF_CALL_WRAPPER2(double_value_index, index_value, std::size_t)
        DEF_CALL_WRAPPER2(float_value_index, index_value, std::size_t)
        DEF_CALL_WRAPPER2(string_value_index, index_value, std::size_t)
        DEF_CALL_WRAPPER2(int_value_index, index_value, std::size_t)

        DEF_CALL_WRAPPER2(start_list_attribute, std::size_t, std::size_t)
        DEF_CALL_WRAPPER1(end_list_attribute, std::size_t)

        DEF_CALL_WRAPPER2(start_map_attribute, std::size_t, std::size_t)
        DEF_CALL_WRAPPER1(end_map_attribute, std::size_t)

        DEF_CALL_WRAPPER3(start_number_list, std::size_t, index_value, std::size_t)
        DEF_CALL_WRAPPER2(number_list_value, std::int64_t, std::size_t)
        DEF_CALL_WRAPPER1(number_list_null_value, std::size_t)
        DEF_CALL_WRAPPER1(end_number_list, std::size_t)

        DEF_CALL_WRAPPER1(points_null_attr, index_value)
        DEF_CALL_WRAPPER3(points_attr, index_value, index_value, int64_t)

#undef DEF_CALL_WRAPPER0
#undef DEF_CALL_WRAPPER1
#undef DEF_CALL_WRAPPER2
#undef DEF_CALL_WRAPPER3
// @endcond

        // No macro for this one, because it is templated on TValue:

        // This overload matches only when the handler has the attribute_value() member function of type TValue.
        template <typename THandler, typename TValue, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().attribute_value(std::declval<TValue>(), std::declval<std::size_t>()))>::value, int>::type = 0>
        bool call_attribute_value(THandler&& handler, TValue value, std::size_t depth) {
            return std::forward<THandler>(handler).attribute_value(value, depth);
        }

        // This overload matches only when the handler has the attribute_value() member function of type TValue.
        template <typename THandler, typename TValue, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().attribute_value(std::declval<TValue>(), std::declval<std::size_t>()))>::value, int>::type = 0>
        bool call_attribute_value(THandler&& handler, TValue value, std::size_t depth) {
            std::forward<THandler>(handler).attribute_value(value, depth);
            return true;
        }

        // This overload matches always.
        template <typename... TArgs>
        bool call_attribute_value(TArgs&&... /*unused*/) {
            return true;
        }

        template <typename TFeaturePtr>
        void throw_format_exception(const std::string& message, const TFeaturePtr feature) {
            throw format_exception{message, feature->layer_num(), feature->feature_num()};
        }

        template <>
        inline void throw_format_exception<std::nullptr_t>(const std::string& message, std::nullptr_t) {
            throw format_exception{message};
        }

        template <typename TFeaturePtr, typename TIterator>
        void skip_structured_value(const TFeaturePtr feature, std::size_t depth, TIterator& it, TIterator end) {
            if (it == end) {
                throw_format_exception<TFeaturePtr>("Attributes list is missing value", feature);
            }

            const uint64_t structured_value = *it++;

            const auto vt = structured_value & 0x0fU;

            // "4.4.2.2. Structured Value Encoding" says "Value types 11 through
            // 15 are reserved for future versions of this specification.
            // Implementations MUST treat structured values of these types as
            // opaque values that consume only one integer of storage"
            if (vt > static_cast<std::size_t>(structured_value_type::max)) {
                return;
            }

            const auto cvt = static_cast<structured_value_type>(vt);

            if (cvt == structured_value_type::cvt_list) {
                auto vp = structured_value >> 4U;
                while (vp > 0) {
                    --vp;
                    skip_structured_value(feature, depth + 1, it, end);
                }
            } else if (cvt == structured_value_type::cvt_map) {
                auto vp = structured_value >> 4U;
                while (vp > 0) {
                    --vp;
                    if (it == end) {
                        throw_format_exception<TFeaturePtr>("Attributes map is missing key", feature);
                    }
                    ++it; // skip key
                    if (it == end) {
                        throw_format_exception<TFeaturePtr>("Attributes map is missing value", feature);
                    }
                    skip_structured_value(feature, depth + 1, it, end);
                }
            } else if (cvt == structured_value_type::cvt_number_list) {
                const auto count = static_cast<int64_t>((structured_value >> 4U) + 1);
                if (std::distance(it, end) < count) {
                    throw_format_exception<TFeaturePtr>("Attributes number-list is incomplete", feature);
                }
                std::advance(it, count);
            }
        }

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_DETAIL_ATTRIBUTES_HPP
