#ifndef VTZERO_UNALIGNED_TABLE_HPP
#define VTZERO_UNALIGNED_TABLE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file unaligned_table.hpp
 *
 * @brief Contains the unaligned_table template class.
 */

#include "exception.hpp"
#include "types.hpp"

#include <cstring>

namespace vtzero {

    namespace detail {

        template <typename T>
        class unaligned_table {

            data_view m_data{};

        public:

            unaligned_table() noexcept = default;

            explicit unaligned_table(data_view data) noexcept :
                m_data(data) {
                vtzero_assert_in_noexcept_function(m_data.size() % sizeof(T) == 0);
            }

            bool empty() const noexcept {
                return m_data.empty();
            }

            std::size_t size() const noexcept {
                return m_data.size() / sizeof(T);
            }

            T at(index_value index) const {
                if (index.value() >= size()) {
                    throw out_of_range_exception{index.value()};
                }
                T result;
                std::memcpy(&result, m_data.data() + index.value() * sizeof(T), sizeof(T));
                return result;
            }

        }; // class unaligned table

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_UNALIGNED_TABLE_HPP
