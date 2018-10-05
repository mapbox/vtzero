#ifndef VTZERO_ATTRIBUTES_HPP
#define VTZERO_ATTRIBUTES_HPP

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

#include "property_value.hpp"
#include "types.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace vtzero {

    namespace detail {

        // attribute value types
        enum class complex_value_type {
            cvt_inline_sint =  0,
            cvt_inline_uint =  1,
            cvt_bool        =  2,
            cvt_null        =  2,
            cvt_float       =  3,
            cvt_double      =  4,
            cvt_string      =  5,
            cvt_sint        =  6,
            cvt_uint        =  7,
            cvt_list        =  8,
            cvt_map         =  9,
            cvt_number_list = 10,
            max             = 10
        };

// @cond internal
        // These macros are used to define "wrapper" functions for calling
        // member functions on a handler class. If the class doesn't define
        // some member function a dummy is called that always returns true.
        // If the class defines the function, it is called.

        // Call a function with one parameters
#define DEF_CALL_WRAPPER1(func) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, std::size_t depth) { \
            return std::forward<THandler>(handler).func(depth); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, std::size_t depth) { \
            std::forward<THandler>(handler).func(depth); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        } \

        // Call a function with two parameters
#define DEF_CALL_WRAPPER2(func, ptype) \
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().func(std::declval<ptype>(), std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype param, std::size_t depth) { \
            return std::forward<THandler>(handler).func(param, depth); \
        } \
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().func(std::declval<ptype>(), std::declval<std::size_t>()))>::value, int>::type = 0> \
        bool call_##func(THandler&& handler, ptype param, std::size_t depth) { \
            std::forward<THandler>(handler).func(param, depth); \
            return true; \
        } \
        template <typename... TArgs> \
        bool call_##func(TArgs&&... /*unused*/) { \
            return true; \
        } \

        DEF_CALL_WRAPPER2(key_index, index_value)
        DEF_CALL_WRAPPER2(attribute_key, data_view)
        DEF_CALL_WRAPPER2(value_index, index_value)
        DEF_CALL_WRAPPER2(double_value_index, index_value)
        DEF_CALL_WRAPPER2(float_value_index, index_value)
        DEF_CALL_WRAPPER2(string_value_index, index_value)
        DEF_CALL_WRAPPER2(int_value_index, index_value)

        DEF_CALL_WRAPPER2(start_list_attribute, std::size_t)
        DEF_CALL_WRAPPER1(end_list_attribute)

        DEF_CALL_WRAPPER2(start_map_attribute, std::size_t)
        DEF_CALL_WRAPPER1(end_map_attribute)

        DEF_CALL_WRAPPER2(number_list_value, std::uint64_t)
        DEF_CALL_WRAPPER1(end_number_list)

#undef DEF_CALL_WRAPPER1
#undef DEF_CALL_WRAPPER2
// @endcond

        // This overload matches only when the handler has the value() member function of type TValue.
        template <typename THandler, typename TValue, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().attribute_value(std::declval<TValue>(), std::declval<std::size_t>()))>::value, int>::type = 0>
        bool call_attribute_value(THandler&& handler, TValue value, std::size_t depth) {
            return std::forward<THandler>(handler).attribute_value(value, depth);
        }

        // This overload matches only when the handler has the value() member function of type TValue.
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

        // This overload matches only when the handler has the start_number_list() member function returning bool.
        template <typename THandler, typename std::enable_if<std::is_same<bool, decltype(std::declval<THandler>().start_number_list(std::declval<std::size_t>(), std::declval<index_value>(), std::declval<std::size_t>()))>::value, int>::type = 0>
        bool call_start_number_list(THandler&& handler, std::size_t count, index_value index, std::size_t depth) {
            return std::forward<THandler>(handler).start_number_list(count, index, depth);
        }

        // This overload matches only when the handler has the start_number_list() member function returning void.
        template <typename THandler, typename std::enable_if<std::is_same<void, decltype(std::declval<THandler>().start_number_list(std::declval<std::size_t>(), std::declval<index_value>(), std::declval<std::size_t>()))>::value, int>::type = 0>
        bool call_start_number_list(THandler&& handler, std::size_t count, index_value index, std::size_t depth) {
            std::forward<THandler>(handler).start_number_list(count, index, depth);
            return true;
        }

        // This overload matches always.
        template <typename... TArgs>
        bool call_start_number_list(TArgs&&... /*unused*/) {
            return true;
        }

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_ATTRIBUTES_HPP
