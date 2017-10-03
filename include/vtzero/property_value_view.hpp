#ifndef VTZERO_PROPERTY_VALUE_VIEW_HPP
#define VTZERO_PROPERTY_VALUE_VIEW_HPP

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <cstring>
#include <utility>

namespace vtzero {

    class property_value_view {

        data_view m_value;

        static bool check_tag_and_type(protozero::pbf_tag_type tag, protozero::pbf_wire_type type) noexcept {
            static protozero::pbf_wire_type types[] = {
                protozero::pbf_wire_type::length_delimited, // dummy 0 value
                string_value_type::wire_type,
                float_value_type::wire_type,
                double_value_type::wire_type,
                int_value_type::wire_type,
                uint_value_type::wire_type,
                sint_value_type::wire_type,
                bool_value_type::wire_type,
            };

            if (tag < 1 || tag > 7) {
                return false;
            }

            return types[tag] == type;
        }

        static data_view get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, string_value_type) {
            return value_message.get_view();
        }

        static float get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, float_value_type) {
            return value_message.get_float();
        }

        static double get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, double_value_type) {
            return value_message.get_double();
        }

        static int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, int_value_type) {
            return value_message.get_int64();
        }

        static uint64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, uint_value_type) {
            return value_message.get_uint64();
        }

        static int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, sint_value_type) {
            return value_message.get_sint64();
        }

        static bool get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, bool_value_type) {
            return value_message.get_bool();
        }

        template <typename T>
        typename T::type get_value() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            decltype(T::value) result;
            bool has_result = false;
            while (value_message.next(T::pvtype, T::wire_type)) {
                result = get_value_impl(value_message, T{});
                has_result = true;
            }

            if (has_result) {
                return result;
            }

            throw type_exception{};
        }

        int compare(const property_value_view& other) const noexcept {
            const int cmp = std::memcmp(m_value.data(),
                                        other.m_value.data(),
                                        std::min(m_value.size(), other.m_value.size()));
            if (cmp == 0) {
                if (m_value.size() == other.m_value.size()) {
                    return 0;
                }
                return m_value.size() < other.m_value.size() ? -1 : 1;
            }
            return cmp;
        }

    public:

        property_value_view() = default;

        explicit property_value_view(const data_view& value) noexcept :
            m_value(value) {
        }

        bool valid() const noexcept {
            return m_value.data() != nullptr;
        }

        property_value_type type() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};
            if (value_message.next()) {
                const auto tag_val = static_cast<protozero::pbf_tag_type>(value_message.tag());
                if (!check_tag_and_type(tag_val, value_message.wire_type())) {
                    throw format_exception{"illegal property value type"};
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

        bool operator==(const property_value_view& other) const noexcept {
            return m_value.size() == other.m_value.size() &&
                   std::memcmp(m_value.data(), other.m_value.data(), m_value.size()) == 0;
        }

        bool operator!=(const property_value_view& other) const noexcept {
            return !(*this == other);
        }

        bool operator<(const property_value_view& other) const noexcept {
            return compare(other) < 0;
        }

        bool operator<=(const property_value_view& other) const noexcept {
            return compare(other) <= 0;
        }

        bool operator>(const property_value_view& other) const noexcept {
            return compare(other) > 0;
        }

        bool operator>=(const property_value_view& other) const noexcept {
            return compare(other) >= 0;
        }

    }; // class value_view

    template <typename V>
    decltype(std::declval<V>()(string_value_type{})) apply_visitor(V&& visitor, const property_value_view& value) {
        switch (value.type()) {
            case property_value_type::string_value:
                return std::forward<V>(visitor)(value.string_value());
            case property_value_type::float_value:
                return std::forward<V>(visitor)(value.float_value());
            case property_value_type::double_value:
                return std::forward<V>(visitor)(value.double_value());
            case property_value_type::int_value:
                return std::forward<V>(visitor)(value.int_value());
            case property_value_type::uint_value:
                return std::forward<V>(visitor)(value.uint_value());
            case property_value_type::sint_value:
                return std::forward<V>(visitor)(value.sint_value());
            default: // case property_value_type::bool_value:
                return std::forward<V>(visitor)(value.bool_value());
        }
    }

    namespace detail {

        template <typename T, typename S>
        struct convert_visitor {

            template <typename V>
            T operator()(V value) const {
                return T{value};
            }

            T operator()(data_view value) const {
                return T{S{value}};
            }

        }; // convert_visitor

    } // namespace detail

    /**
     * Convert a property_value_view to a different (usually variant-based)
     * class.
     *
     * Usage: If you have a variant type like
     *
     * @code
     *   using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
     * @endcode
     *
     * you can use
     * @code
     *   property_value_view x = ...;
     *   auto v = convert_property_value<variant_type>(x);
     * @endcode
     *
     * to convert the data.
     *
     * @tparam T The variant type to convert to. Must contain the types float,
     *           double, int64_t, uint64_t, and bool plus the string type S.
     * @tparam S The string type to use in the variant. By default this is
     *           std::string, but you can use anything that is convertible
     *           from a vtzero::data_view.
     * @param value The property value to convert.
     *
     */
    template <typename T, typename S = std::string>
    T convert_property_value(const property_value_view& value) {
        return apply_visitor(detail::convert_visitor<T, S>{}, value);
    }

} // namespace vtzero

#endif // VTZERO_PROPERTY_VALUE_VIEW_HPP
