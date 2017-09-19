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
        decltype(T::value) get_value(detail::pbf_value type, protozero::pbf_wire_type wire_type) const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            decltype(T::value) result;
            bool has_result = false;
            while (value_message.next(type, wire_type)) {
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

        value_view(const data_view& value) noexcept :
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
            return get_value<string_value_type>(detail::pbf_value::string_value,
                                                protozero::pbf_wire_type::length_delimited);
        }

        float float_value() const {
            return get_value<float_value_type>(detail::pbf_value::float_value,
                                               protozero::pbf_wire_type::fixed32);
        }

        double double_value() const {
            return get_value<double_value_type>(detail::pbf_value::double_value,
                                                protozero::pbf_wire_type::fixed64);
        }

        std::int64_t int_value() const {
            return get_value<int_value_type>(detail::pbf_value::int_value,
                                             protozero::pbf_wire_type::varint);
        }

        std::uint64_t uint_value() const {
            return get_value<uint_value_type>(detail::pbf_value::uint_value,
                                              protozero::pbf_wire_type::varint);
        }

        std::int64_t sint_value() const {
            return get_value<sint_value_type>(detail::pbf_value::sint_value,
                                              protozero::pbf_wire_type::varint);
        }

        bool bool_value() const {
            return get_value<bool_value_type>(detail::pbf_value::bool_value,
                                              protozero::pbf_wire_type::varint);
        }

    }; // class value_view

    template <typename V>
    void value_visit(V&& visitor, const value_view& value) {
        switch (value.type()) {
            case detail::pbf_value::string_value:
                std::forward<V>(visitor)(value.string_value());
                break;
            case detail::pbf_value::float_value:
                std::forward<V>(visitor)(value.float_value());
                break;
            case detail::pbf_value::double_value:
                std::forward<V>(visitor)(value.double_value());
                break;
            case detail::pbf_value::int_value:
                std::forward<V>(visitor)(value.int_value());
                break;
            case detail::pbf_value::uint_value:
                std::forward<V>(visitor)(value.uint_value());
                break;
            case detail::pbf_value::sint_value:
                std::forward<V>(visitor)(value.sint_value());
                break;
            case detail::pbf_value::bool_value:
                std::forward<V>(visitor)(value.bool_value());
                break;
        }
    }

} // namespace vtzero

#endif // VTZERO_VALUE_VIEW_HPP
