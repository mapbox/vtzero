#ifndef VTZERO_VALUE_VIEW_HPP
#define VTZERO_VALUE_VIEW_HPP

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <utility>

namespace vtzero {

    namespace detail {

        inline data_view get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, string_value_type) {
            return value_message.get_view();
        }

        inline float get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, float_value_type) {
            return value_message.get_float();
        }

        inline double get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, double_value_type) {
            return value_message.get_double();
        }

        inline int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, int_value_type) {
            return value_message.get_int64();
        }

        inline uint64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, uint_value_type) {
            return value_message.get_uint64();
        }

        inline int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, sint_value_type) {
            return value_message.get_sint64();
        }

        inline bool get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, bool_value_type) {
            return value_message.get_bool();
        }

    } // namespace detail

    class value_view {

        data_view m_value;

        static bool check_tag_and_type(protozero::pbf_tag_type tag, protozero::pbf_wire_type type) noexcept {
            static protozero::pbf_wire_type types[] = {
                protozero::pbf_wire_type::length_delimited,
                protozero::pbf_wire_type::length_delimited, // string_value
                protozero::pbf_wire_type::fixed32, // float_value
                protozero::pbf_wire_type::fixed64, // double_value
                protozero::pbf_wire_type::varint,  // int_value
                protozero::pbf_wire_type::varint,  // uint_value
                protozero::pbf_wire_type::varint,  // sint_value
                protozero::pbf_wire_type::varint   // bool_value
            };

            if (tag < 1 || tag > 7) {
                return false;
            }

            return types[tag] == type;
        }

        template <typename T>
        typename T::type get_value() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            decltype(T::value) result;
            bool has_result = false;
            while (value_message.next(T::vtype, T::wire_type)) {
                result = detail::get_value_impl(value_message, T{});
                has_result = true;
            }

            if (has_result) {
                return result;
            }

            throw type_exception{};
        }

        protozero::pbf_message<detail::pbf_value> check_value(detail::pbf_value type, protozero::pbf_wire_type wire_type) const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            if (value_message.next(type, wire_type)) {
                return value_message;
            }

            throw type_exception{};
        }

    public:

        value_view() = default;

        explicit value_view(const data_view& value) noexcept :
            m_value(value) {
        }

        bool valid() const noexcept {
            return m_value.data() != nullptr;
        }

        value_type type() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};
            if (value_message.next()) {
                const auto tag_val = static_cast<protozero::pbf_tag_type>(value_message.tag());
                if (!check_tag_and_type(tag_val, value_message.wire_type())) {
                    throw format_exception{"illegal value_type"};
                }
                return value_message.tag();
            }
            throw format_exception{"missing tag value"};
        }

        data_view data() const noexcept {
            return m_value;
        }

        data_view string_value() const {
            return get_value<string_value_type>();
        }

        float float_value() const {
            return get_value<float_value_type>();
        }

        double double_value() const {
            return get_value<double_value_type>();
        }

        std::int64_t int_value() const {
            return get_value<int_value_type>();
        }

        std::uint64_t uint_value() const {
            return get_value<uint_value_type>();
        }

        std::int64_t sint_value() const {
            return get_value<sint_value_type>();
        }

        bool bool_value() const {
            return get_value<bool_value_type>();
        }

    }; // class value_view

    template <typename V>
    decltype(std::declval<V>()(string_value_type{})) value_visit(V&& visitor, const value_view& value) {
        switch (value.type()) {
            case value_type::string_value:
                return std::forward<V>(visitor)(value.string_value());
            case value_type::float_value:
                return std::forward<V>(visitor)(value.float_value());
            case value_type::double_value:
                return std::forward<V>(visitor)(value.double_value());
            case value_type::int_value:
                return std::forward<V>(visitor)(value.int_value());
            case value_type::uint_value:
                return std::forward<V>(visitor)(value.uint_value());
            case value_type::sint_value:
                return std::forward<V>(visitor)(value.sint_value());
            default: // case value_type::bool_value:
                return std::forward<V>(visitor)(value.bool_value());
        }
    }

} // namespace vtzero

#endif // VTZERO_VALUE_VIEW_HPP
