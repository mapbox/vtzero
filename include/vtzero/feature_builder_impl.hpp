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

        // Features are built in these stages. The feature builder starts
        // out in stage "want_id". After that the builder progresses through
        // (some of) the other stages until it is "done".
        enum class stage {
            want_id         = 0, // start stage, waiting for ID
            has_id          = 1, // ID has been set
            want_geometry   = 2, // geometry is being set
            has_geometry    = 3, // full geometry has been set
            want_tags       = 4, // tags (version 2) are being set
            want_attrs      = 5, // attributes (version 3) are being set
            want_geom_attrs = 6, // geometric attributes are being set
            done            = 7  // after commit or rollback
        };

        class feature_builder_base {

            layer_builder_impl* m_layer;

            void set_integer_id_impl(const uint64_t id) {
                m_feature_writer.add_uint64(detail::pbf_feature::id, id);
            }

            void set_string_id_impl(const data_view& id) {
                m_feature_writer.add_string(detail::pbf_feature::string_id, id);
            }

        protected:

            protozero::pbf_builder<detail::pbf_feature> m_feature_writer;
            protozero::packed_field_uint64 m_pbf_attributes;

            stage m_stage = stage::want_id;

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

            /// Helper function doing subtraction with no integer overflow possible.
            static int32_t sub(const int32_t a, const int32_t b) noexcept {
                return static_cast<int32_t>(static_cast<int64_t>(a) -
                                            static_cast<int64_t>(b));
            }

            uint32_t version() const noexcept {
                return m_layer->version();
            }

            std::vector<int32_t>& elevations() noexcept {
                return m_layer->elevations();
            }

            std::vector<uint64_t>& knots() noexcept {
                return m_layer->knots();
            }

            void add_key_internal(index_value idx) {
                vtzero_assert(idx.valid());
                m_pbf_attributes.add_element(idx.value());
            }

            template <typename T>
            void add_key_internal(T&& key) {
                add_key_internal(m_layer->add_key(data_view{std::forward<T>(key)}));
            }

            void add_complex_value(complex_value_type type, uint64_t param) {
                m_pbf_attributes.add_element(create_complex_value(type, param));
            }

            void add_direct_value(uint64_t value) {
                m_pbf_attributes.add_element(value);
            }

            void add_value_internal_vt2(index_value idx) {
                vtzero_assert(idx.valid());
                m_pbf_attributes.add_element(idx.value());
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

            void add_value_internal_vt3(const std::string& text) {
                vtzero_assert(m_stage == stage::want_attrs || m_stage == stage::want_geom_attrs);
                data_view value{text};
                const auto idx = m_layer->add_string_value(value);
                add_complex_value(detail::complex_value_type::cvt_string, idx.value());
            }

            void add_property_impl_vt2(const property& property) {
                vtzero_assert(m_stage == stage::want_tags);
                add_key_internal(property.key());
                add_value_internal_vt2(property.value());
            }

            void add_property_impl_vt2(const index_value_pair idxs) {
                vtzero_assert(m_stage == stage::want_tags);
                add_key_internal(idxs.key());
                add_value_internal_vt2(idxs.value());
            }

            template <typename TKey, typename TValue>
            void add_property_impl_vt2(TKey&& key, TValue&& value) {
                vtzero_assert(m_stage == stage::want_tags);
                add_key_internal(std::forward<TKey>(key));
                add_value_internal_vt2(std::forward<TValue>(value));
            }

            void do_commit() {
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.commit();
                }
                m_feature_writer.commit();
                m_layer->increment_feature_count();
                m_stage = stage::done;
            }

            void do_rollback() {
                if (m_pbf_attributes.valid()) {
                    m_pbf_attributes.rollback();
                }
                m_feature_writer.rollback();
                m_stage = stage::done;
            }

        public:

            /**
             * Set the integer ID of this feature.
             *
             * @param id The ID.
             *
             * @pre The feature_builder must be in the "want_id" stage.
             * @post The feature_builder is in the "has_id" stage.
             */
            void set_integer_id(const uint64_t id) {
                vtzero_assert(m_stage == stage::want_id && "Must be in 'want_id' stage to call set_integer_id()");
                set_integer_id_impl(id);
                m_stage = stage::has_id;
            }

            /**
             * Set the string ID of this feature.
             *
             * @param id The ID.
             *
             * @pre Layer version is 3.
             * @pre The feature_builder must be in the "want_id" stage.
             * @post The feature_builder is in the "has_id" stage.
             */
            void set_string_id(const data_view& id) {
                vtzero_assert(version() == 3 && "string_id is only allowed in version 3 layers");
                vtzero_assert(m_stage == stage::want_id && "Must be in 'want_id' stage to call set_string_id()");
                set_string_id_impl(id);
                m_stage = stage::has_id;
            }

            /**
             * Copy the ID of an existing feature to this feature. If the
             * feature doesn't have an ID, no ID is set.
             *
             * If you are building a version 2 layer, a string_id will not
             * be copied!
             *
             * @param feature The feature to copy the ID from.
             *
             * @pre The feature_builder must be in the "want_id" stage.
             * @post The feature_builder is in the "has_id" stage.
             */
            void copy_id(const feature& feature) {
                vtzero_assert(m_stage == stage::want_id && "Must be in 'want_id' stage to call copy_id()");
                if (feature.has_integer_id()) {
                    set_integer_id_impl(feature.id());
                } else if (version() == 3 && feature.has_string_id()) {
                    set_string_id_impl(feature.string_id());
                }
                m_stage = stage::has_id;
            }

        }; // class feature_builder_base

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_FEATURE_BUILDER_IMPL_HPP
