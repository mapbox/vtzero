#ifndef VTZERO_FEATURE_BUILDER_IMPL_HPP
#define VTZERO_FEATURE_BUILDER_IMPL_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file feature_builder_impl.hpp
 *
 * @brief Contains classes internal to the builder.
 */

#include "builder_impl.hpp"
#include "encoded_property_value.hpp"
#include "geometry.hpp"
#include "property.hpp"
#include "property_value.hpp"

#include <utility>

namespace vtzero {

    namespace detail {

        class feature_builder_base {

            layer_builder_impl* m_layer;

            void add_key_internal(index_value idx) {
                m_pbf_tags.add_element(idx.value());
            }

            template <typename T>
            void add_key_internal(T&& key) {
                add_key_internal(m_layer->add_key(data_view{std::forward<T>(key)}));
            }

            void add_value_internal(index_value idx) {
                m_pbf_tags.add_element(idx.value());
            }

            void add_value_internal(property_value value) {
                add_value_internal(m_layer->add_value(value.data()));
            }

            template <typename T>
            void add_value_internal(T&& value) {
                encoded_property_value v{std::forward<T>(value)};
                add_value_internal(m_layer->add_value(v.data()));
            }

        protected:

            protozero::pbf_builder<detail::pbf_feature> m_feature_writer;
            protozero::packed_field_uint32 m_pbf_tags;

            explicit feature_builder_base(layer_builder_impl* layer) :
                m_layer(layer),
                m_feature_writer(layer->message(), detail::pbf_layer::features) {
            }

            ~feature_builder_base() noexcept = default;

            feature_builder_base(const feature_builder_base&) = delete; // NOLINT clang-tidy: hicpp-use-equals-delete

            feature_builder_base& operator=(const feature_builder_base&) = delete; // NOLINT clang-tidy: hicpp-use-equals-delete
                                                                                   // The check wants these functions to be public...

            feature_builder_base(feature_builder_base&&) noexcept = default;

            feature_builder_base& operator=(feature_builder_base&&) noexcept = default;

            void add_property_impl(const property& property) {
                add_key_internal(property.key());
                add_value_internal(property.value());
            }

            template <typename TKey, typename TValue>
            void add_property_impl(TKey&& key, TValue&& value) {
                add_key_internal(std::forward<TKey>(key));
                add_value_internal(std::forward<TValue>(value));
            }

            void do_commit() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                if (m_feature_writer.valid()) {
                    m_feature_writer.commit();
                    m_layer->increment_feature_count();
                }
            }

            void do_rollback() {
                if (m_pbf_tags.valid()) {
                    m_pbf_tags.commit();
                }
                m_feature_writer.rollback();
            }

        }; // class feature_builder_base

        class feature_builder : public feature_builder_base {

            class countdown_value {

                uint32_t m_value = 0;

            public:

                countdown_value() noexcept = default;

                ~countdown_value() noexcept {
                    assert_is_zero();
                }

                countdown_value(const countdown_value&) = delete;

                countdown_value& operator=(const countdown_value&) = delete;

                countdown_value(countdown_value&& other) noexcept :
                    m_value(other.m_value) {
                    other.m_value = 0;
                }

                countdown_value& operator=(countdown_value&& other) noexcept {
                    m_value = other.m_value;
                    other.m_value = 0;
                    return *this;
                }

                uint32_t value() const noexcept {
                    return m_value;
                }

                void set(const uint32_t value) noexcept {
                    m_value = value;
                }

                void decrement() noexcept {
                    vtzero_assert_in_noexcept_function(m_value > 0 &&
                                                       "too many calls to set_point()");
                    --m_value;
                }

                void assert_is_zero() const noexcept {
                    vtzero_assert_in_noexcept_function(m_value == 0 &&
                                                       "not enough calls to set_point()");
                }

            }; // countdown_value

        protected:

            protozero::packed_field_uint32 m_pbf_geometry{};
            countdown_value m_num_points;
            point m_cursor{0, 0};

        public:

            explicit feature_builder(layer_builder_impl* layer) :
                feature_builder_base(layer) {
            }

            ~feature_builder() {
                commit(); // XXX exceptions?
            }

            // Builder classes can not be copied
            feature_builder(const feature_builder&) = delete;

            // Builder classes can not be copied
            feature_builder& operator=(const feature_builder&) = delete;

            // Builder classes can be moved
            feature_builder(feature_builder&& other) noexcept = default;

            // Builder classes can be moved
            feature_builder& operator=(feature_builder&& other) noexcept = default;

            void set_id(uint64_t id) {
                vtzero_assert(!m_pbf_geometry.valid());
                m_feature_writer.add_uint64(detail::pbf_feature::id, id);
            }

            void init_geometry() {
                if (!m_pbf_geometry.valid()) {
                    m_pbf_geometry = {m_feature_writer, detail::pbf_feature::geometry};
                }
            }

            template <typename ...TArgs>
            void add_property(TArgs&& ...args) {
                if (m_pbf_geometry.valid()) {
                    m_num_points.assert_is_zero();
                    m_pbf_geometry.commit();
                }
                if (!m_pbf_tags.valid()) {
                    m_pbf_tags = {m_feature_writer, detail::pbf_feature::tags};
                }
                add_property_impl(std::forward<TArgs>(args)...);
            }

            void commit() {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                }
                do_commit();
            }

            void rollback() {
                if (m_pbf_geometry.valid()) {
                    m_pbf_geometry.commit();
                }
                do_rollback();
            }

        }; // class feature_builder

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_FEATURE_BUILDER_IMPL_HPP
