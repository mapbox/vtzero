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
            std::size_t m_layer_num;

        public:

            unaligned_table() noexcept = default;

            explicit unaligned_table(data_view data, std::size_t layer_num) :
                m_data(data),
                m_layer_num(layer_num) {
                if (data.size() % sizeof(T) != 0) {
                    throw format_exception{"Value table in layer has invalid size", layer_num};
                }
            }

            bool empty() const noexcept {
                return m_data.empty();
            }

            std::size_t size() const noexcept {
                return m_data.size() / sizeof(T);
            }

            T at(index_value index) const {
                if (index.value() >= size()) {
                    throw out_of_range_exception{index.value(), m_layer_num};
                }
                T result;
                std::memcpy(&result, m_data.data() + index.value() * sizeof(T), sizeof(T));
                return result;
            }

        }; // class unaligned_table

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_UNALIGNED_TABLE_HPP
