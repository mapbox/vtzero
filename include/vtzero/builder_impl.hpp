#ifndef VTZERO_BUILDER_IMPL_HPP
#define VTZERO_BUILDER_IMPL_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file builder_impl.hpp
 *
 * @brief Contains classes internal to the builder.
 */

#include "types.hpp"

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_message.hpp>

#include <cstdlib>
#include <string>
#include <utility>

namespace vtzero {

    namespace detail {

        class layer_builder_base {

        public:

            layer_builder_base() noexcept = default;

            virtual ~layer_builder_base() noexcept = default;

            layer_builder_base(const layer_builder_base&) noexcept = default;
            layer_builder_base& operator=(const layer_builder_base&) noexcept = default;

            layer_builder_base(layer_builder_base&&) noexcept = default;
            layer_builder_base& operator=(layer_builder_base&&) noexcept = default;

            virtual std::size_t estimated_size() const = 0;

            virtual void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const = 0;

        }; // class layer_builder_base

        class layer_builder_impl : public layer_builder_base {

            std::string m_data;
            std::string m_keys_data;
            std::string m_values_data;

            protozero::pbf_builder<detail::pbf_layer> m_pbf_message_layer;
            protozero::pbf_builder<detail::pbf_layer> m_pbf_message_keys;
            protozero::pbf_builder<detail::pbf_layer> m_pbf_message_values;

            std::size_t m_num_features = 0;
            uint32_t m_max_key = 0;
            uint32_t m_max_value = 0;

            static index_value find_in_table(const data_view text, const std::string& data) {
                uint32_t index = 0;
                protozero::pbf_message<detail::pbf_layer> pbf_table{data};

                while (pbf_table.next()) {
                    const auto v = pbf_table.get_view();
                    if (v == text) {
                        return index_value{index};
                    }
                    ++index;
                }

                return index_value{};
            }

        public:

            template <typename TString>
            layer_builder_impl(TString&& name, uint32_t version, uint32_t extent) :
                m_pbf_message_layer(m_data),
                m_pbf_message_keys(m_keys_data),
                m_pbf_message_values(m_values_data) {
                m_pbf_message_layer.add_uint32(detail::pbf_layer::version, version);
                m_pbf_message_layer.add_string(detail::pbf_layer::name, std::forward<TString>(name));
                m_pbf_message_layer.add_uint32(detail::pbf_layer::extent, extent);
            }

            ~layer_builder_impl() noexcept override = default;

            layer_builder_impl(const layer_builder_impl&) = delete;
            layer_builder_impl& operator=(const layer_builder_impl&) = delete;

            layer_builder_impl(layer_builder_impl&&) = default;
            layer_builder_impl& operator=(layer_builder_impl&&) = default;

            index_value add_key_without_dup_check(const data_view text) {
                m_pbf_message_keys.add_string(detail::pbf_layer::keys, text);
                return m_max_key++;
            }

            index_value add_key(const data_view text) {
                const auto index = find_in_table(text, m_keys_data);
                if (index.valid()) {
                    return index;
                }
                return add_key_without_dup_check(text);
            }

            index_value add_value_without_dup_check(const data_view text) {
                m_pbf_message_values.add_string(detail::pbf_layer::values, text);
                return m_max_value++;
            }

            index_value add_value(const data_view text) {
                const auto index = find_in_table(text, m_values_data);
                if (index.valid()) {
                    return index;
                }
                return add_value_without_dup_check(text);
            }

            const std::string& data() const noexcept {
                return m_data;
            }

            const std::string& keys_data() const noexcept {
                return m_keys_data;
            }

            const std::string& values_data() const noexcept {
                return m_values_data;
            }

            protozero::pbf_builder<detail::pbf_layer>& message() noexcept {
                return m_pbf_message_layer;
            }

            void increment_feature_count() noexcept {
                ++m_num_features;
            }

            std::size_t estimated_size() const override {
                constexpr const std::size_t estimated_overhead_for_pbf_encoding = 8;
                return data().size() +
                    keys_data().size() +
                    values_data().size() +
                    estimated_overhead_for_pbf_encoding;
            }

            void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const override {
                if (m_num_features > 0) {
                    pbf_tile_builder.add_bytes_vectored(detail::pbf_tile::layers,
                        data(),
                        keys_data(),
                        values_data()
                    );
                }
            }

        }; // class layer_builder_impl

        class layer_builder_existing : public layer_builder_base {

            data_view m_data;

        public:

            explicit layer_builder_existing(const data_view data) :
                m_data(data) {
            }

            std::size_t estimated_size() const override {
                constexpr const std::size_t estimated_overhead_for_pbf_encoding = 8;
                return m_data.size() + estimated_overhead_for_pbf_encoding;
            }

            void build(protozero::pbf_builder<detail::pbf_tile>& pbf_tile_builder) const override {
                pbf_tile_builder.add_bytes(detail::pbf_tile::layers, m_data);
            }

        }; // class layer_builder_existing

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_BUILDER_IMPL_HPP
