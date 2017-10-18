#ifndef VTZERO_INDEX_HPP
#define VTZERO_INDEX_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file index.hpp
 *
 * @brief Contains classes for indexing the key/value tables inside layers.
 */

#include "builder.hpp"

#include <cstdint>

namespace vtzero {

    template <template<typename...> class TMap>
    class key_index {

        layer_builder& m_builder;

        TMap<std::string, index_value> m_index;

    public:

        explicit key_index(layer_builder& builder) :
            m_builder(builder) {
        }

        index_value operator()(const data_view v) {
            std::string text(v);
            const auto it = m_index.find(text);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_key_without_dup_check(v);
                m_index.emplace(std::move(text), idx);
                return idx;
            }
            return it->second;
        }

    }; // class key_index

    template <typename TInternal, typename TExternal, template<typename...> class TMap>
    class value_index {

        layer_builder& m_builder;

        TMap<TExternal, index_value> m_index;

    public:

        explicit value_index(layer_builder& builder) :
            m_builder(builder) {
        }

        index_value operator()(const TExternal& v) {
            const auto it = m_index.find(v);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_value_without_dup_check(encoded_property_value{TInternal{v}}.data());
                m_index.emplace(v, idx);
                return idx;
            }
            return it->second;
        }

    }; // class value_index

    template <template<typename...> class TMap>
    class value_index_internal {

        layer_builder& m_builder;

        TMap<encoded_property_value, index_value> m_index;

    public:

        explicit value_index_internal(layer_builder& builder) :
            m_builder(builder) {
        }

        index_value operator()(const encoded_property_value& v) {
            const auto it = m_index.find(v);
            if (it == m_index.end()) {
                const auto idx = m_builder.add_value_without_dup_check(v.data());
                m_index.emplace(v, idx);
                return idx;
            }
            return it->second;
        }

    }; // class value_index_internal

} // namespace vtzero

#endif // VTZERO_INDEX_HPP
