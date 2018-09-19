#ifndef VTZERO_UTIL_HPP
#define VTZERO_UTIL_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file util.hpp
 *
 * @brief Contains utility functions.
 */

#include <type_traits>
#include <utility>

namespace vtzero {

    namespace detail {

        template <typename T, typename Enable = void>
        struct get_result {

            using type = void;

            template <typename THandler>
            static void of(THandler&& /*handler*/) noexcept {
            }

        };

        template <typename T>
        struct get_result<T, typename std::enable_if<!std::is_same<decltype(std::declval<T>().result()), void>::value>::type> {

            using type = decltype(std::declval<T>().result());

            template <typename THandler>
            static type of(THandler&& handler) {
                return std::forward<THandler>(handler).result();
            }

        };

        template <typename T>
        using get_result_t = typename get_result<T>::type;

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_UTIL_HPP
