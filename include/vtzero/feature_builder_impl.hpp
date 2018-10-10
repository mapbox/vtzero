#ifndef VTZERO_FEATURE_BUILDER_IMPL_HPP
#define VTZERO_FEATURE_BUILDER_IMPL_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file feature_builder_impl.hpp
 *
 * @brief Contains classes internal to the builder.
 */

#include "attributes.hpp"
#include "builder_impl.hpp"
#include "encoded_property_value.hpp"
#include "feature.hpp"
#include "geometry.hpp"
#include "property.hpp"
#include "property_value.hpp"

#include <type_traits>
#include <utility>

namespace vtzero {

    namespace detail {

        // Features are built in these stages. The feature builder start
        // out in stage "id". After an integer or string id has been set,
        // it goes to stage "has_id". Any code adding (parts of) a geometry
        // will switch it to the "geometry" stage and any code adding
        // attributes will switch it to the "attributes" stage. After a
        // commit or rollback it will be in the "done" stage.
        enum class stage {
            id         = 0,
            has_id     = 1,
            geometry   = 2,
            attributes = 3,
            done       = 4
        };

        class feature_builder_base {

            layer_builder_impl* m_layer;

        protected:

            protozero::pbf_builder<detail::pbf_feature> m_feature_writer;
            protozero::packed_field_uint32 m_pbf_tags;
            protozero::packed_field_uint64 m_pbf_attributes;

            stage m_stage = stage::id;

            explicit feature_builder_base(layer_builder_impl* layer) :
                m_layer(layer),
                m_feature_writer(layer->message(), detail::pbf_layer::features) {
            }

            ~feature_builder_base() noexcept = default;

            feature_builder_base(const feature_builder_base&) = delete; // NOLINT(hicpp-use-equals-delete, modernize-use-equals-delete)

            feature_builder_base& operator=(const feature_builder_base&) = delete; // NOLINT(hicpp-use-equals-delete, modernize-use-equals-delete)
                                                                                   // The check wants these functions to be public...

            feature_builder_base(feature_builder_base&&) noexcept = default;

            feature_builder_base& operator=(feature_builder_base&&) noexcept = default;

            /// Helper function to make sure we have everything before adding a property
            void prepare_to_add_property_vt2() {
                vtzero_assert(m_stage == stage::attributes);
                if (!m_pbf_tags.valid()) {
                    m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
                }
            }

            /// Helper function to make sure we have everything before adding a property
            void prepare_to_add_property_vt3() {
                vtzero_assert(m_stage == stage::attributes);
                if (!m_pbf_attributes.valid()) {
                    m_pbf_attributes = {m_feature_writer, detail::pbf_feature::attributes};
                }
            }

            void add_key_internal(index_value idx) {
                vtzero_assert(idx.valid());
                if (version() < 3) {
                    prepare_to_add_property_vt2();
                    m_pbf_tags.add_element(idx.value());
                } else {
                    prepare_to_add_property_vt3();
                    m_pbf_attributes.add_element(idx.value());
                }
            }

            template <typename T>
            void add_key_internal(T&& key) {
                add_key_internal(m_layer->add_key(data_view{std::forward<T>(key)}));
            }

            void add_complex_value(detail::complex_value_type type, uint64_t param) {
                m_pbf_attributes.add_element(static_cast<uint64_t>(type) | (param << 4u));
            }

            void add_direct_value(uint64_t value) {
                m_pbf_attributes.add_element(value);
            }

            void add_value_internal_vt2(index_value idx) {
                vtzero_assert(idx.valid());
                m_pbf_tags.add_element(idx.value());
            }

            void add_value_internal_vt2(property_value value) {
                add_value_internal_vt2(m_layer->add_value(value));
            }

            template <typename T>
            void add_value_internal_vt2(T&& value) {
                encoded_property_value v{std::forward<T>(value)};
                add_value_internal_vt2(m_layer->add_value(v));
            }

            template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, int>::type = 0>
            void add_value_internal_vt3(T value) {
                const auto raw_value = static_cast<uint64_t>(value);
                if (raw_value < (1ull << 60u)) {
                    add_complex_value(detail::complex_value_type::cvt_inline_uint, raw_value);
                } else {
                    const auto idx = m_layer->add_int_value(raw_value);
                    add_complex_value(detail::complex_value_type::cvt_uint, idx.value());
                }
            }

            template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, int>::type = 0>
            void add_value_internal_vt3(T value) {
                const auto raw_value = protozero::encode_zigzag64(static_cast<int64_t>(value));
                if (raw_value < (1ull << 60u)) {
                    add_complex_value(detail::complex_value_type::cvt_inline_sint, raw_value);
                } else {
                    const auto idx = m_layer->add_int_value(raw_value);
                    add_complex_value(detail::complex_value_type::cvt_sint, idx.value());
                }
            }

            void add_value_internal_vt3(bool value) {
                add_complex_value(detail::complex_value_type::cvt_bool, value ? 1 : 0);
            }

            void add_value_internal_vt3(null_type /*unused*/) {
                add_complex_value(detail::complex_value_type::cvt_null, null_type::encoded_value);
            }

            void add_value_internal_vt3(double value) {
                const auto idx = m_layer->add_double_value(value);
                add_complex_value(detail::complex_value_type::cvt_double, idx.value());
            }

            void add_value_internal_vt3(float value) {
                const auto idx = m_layer->add_float_value(value);
                add_complex_value(detail::complex_value_type::cvt_float, idx.value());
            }

            void add_value_internal_vt3(const data_view& value) {
                const auto idx = m_layer->add_string_value(value);
                add_complex_value(detail::complex_value_type::cvt_string, idx.value());
            }

            void add_value_internal_vt3(const char* text) {
                data_view value{text};
                const auto idx = m_layer->add_string_value(value);
                add_complex_value(detail::complex_value_type::cvt_string, idx.value());
            }

            uint32_t version() const noexcept {
                return m_layer->version();
            }

            void set_integer_id_impl(uint64_t id) {
                m_feature_writer.add_uint64(detail::pbf_feature::id, id);
            }

            void set_string_id_impl(data_view id) {
                m_feature_writer.add_string(detail::pbf_feature::string_id, id);
            }

            void copy_id_impl(const feature& feature) {
                if (feature.has_integer_id()) {
                    set_integer_id_impl(feature.id());
                } else if (feature.has_string_id()) {
                    set_string_id_impl(feature.string_id());
                }
            }

            void add_value_internal_vt3(const std::string& text) {
                vtzero_assert(m_stage == stage::attributes);
                data_view value{text};
                const auto idx = m_layer->add_string_value(value);
                add_complex_value(detail::complex_value_type::cvt_string, idx.value());
            }

            void add_property_impl_vt2(const property& property) {
                vtzero_assert(m_stage == stage::attributes);
                add_key_internal(property.key());
                add_value_internal_vt2(property.value());
            }

            void add_property_impl_vt2(const index_value_pair idxs) {
                vtzero_assert(m_stage == stage::attributes);
                add_key_internal(idxs.key());
                add_value_internal_vt2(idxs.value());
            }

            template <typename TKey, typename TValue>
            void add_property_impl_vt2(TKey&& key, TValue&& value) {
                vtzero_assert(m_stage == stage::attributes);
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt2(std::forward<TValue>(value));
            }

            template <typename TKey, typename TValue>
            void add_property_impl_vt3(TKey&& key, TValue&& value) {
                vtzero_assert(m_stage == stage::attributes);
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt3(std::forward<TValue>(value));
            }

            void do_commit() {
                vtzero_assert(m_stage == stage::geometry || m_stage == stage::attributes);
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.commit();
                }
                m_feature_writer.commit();
                m_layer->increment_feature_count();
                m_stage = stage::done;
            }

            void do_rollback() {
                vtzero_assert(m_stage != stage::done);
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.rollback();
                }
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.rollback();
                }
                m_feature_writer.rollback();
                m_stage = stage::done;
            }

        }; // class feature_builder_base

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_FEATURE_BUILDER_IMPL_HPP
