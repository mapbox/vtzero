#ifndef VTZERO_LAYER_TABLE_HPP
#define VTZERO_LAYER_TABLE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file layer_table.hpp
 *
 * @brief Contains the layer_table template class.
 */

#include "exception.hpp"
#include "types.hpp"

#include <cstring>

namespace vtzero {

    /**
     * Lookup table in layers.
     *
     * These are returned by the layer::double_table(), layer::float_table(),
     * and layer::int_table() functions.
     *
     * Do not instantiate this class yourself.
     */
    template <typename T>
    class layer_table {

        data_view m_data{};
        std::size_t m_layer_num = 0;

    public:

        layer_table() noexcept = default;

        /// Constructor
        layer_table(data_view data, std::size_t layer_num) :
            m_data(data),
            m_layer_num(layer_num) {
            if (data.size() % sizeof(T) != 0) {
                throw format_exception{"Value table in layer has invalid size", layer_num};
            }
        }

        /// Is this table empty?
        bool empty() const noexcept {
            return m_data.empty();
        }

        /// Return size of the table.
        std::size_t size() const noexcept {
            return m_data.size() / sizeof(T);
        }

        /**
         * Return value at specified index.
         *
         * @param index Index in table
         * @returns value at index
         * @throws out_of_range_exception if the index was out of bounds
         */
        T at(index_value index) const {
            if (index.value() >= size()) {
                throw out_of_range_exception{index.value(), m_layer_num};
            }
            T result;
            std::memcpy(&result, m_data.data() + index.value() * sizeof(T), sizeof(T));
            return result;
        }

    }; // class layer_table

} // namespace vtzero

#endif // VTZERO_LAYER_TABLE_HPP
