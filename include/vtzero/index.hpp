#ifndef VTZERO_INDEX_HPP
#define VTZERO_INDEX_HPP

#include "builder.hpp"

#include <cstdint>
#include <unordered_map>

namespace vtzero {

    class key_index_unordered_map {

        layer_builder& m_builder;

        std::unordered_map<std::string, index_value> m_index;

    public:

        key_index_unordered_map(layer_builder& builder) :
            m_builder(builder) {
        }

        index_value operator()(const data_view& v) {
            std::string text{v};
            const auto it = m_index.find(text);
            if (it == m_index.end()) {
                const auto idx = m_builder.get_layer().add_key_without_dup_check(v);
                m_index.emplace(std::move(text), idx);
                return idx;
            }
            return it->second;
        }

    }; // class key_index_unordered_map

    template <typename TInternal, typename TExt>
    class value_index_unordered_map {

        layer_builder& m_builder;

        std::unordered_map<TExt, index_value> m_index;

    public:

        value_index_unordered_map(layer_builder& builder) :
            m_builder(builder) {
        }

        index_value operator()(const TExt& v) {
            const auto it = m_index.find(v);
            if (it == m_index.end()) {
                const auto idx = m_builder.get_layer().add_value_without_dup_check(property_value{TInternal{v}}.data());
                m_index.emplace(v, idx);
                return idx;
            }
            return it->second;
        }

    }; // class value_index_unordered_map

} // namespace vtzero

#endif // VTZERO_INDEX_HPP
