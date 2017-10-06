#ifndef VTZERO_PROPERTY_VALUE_VIEW_HPP
#define VTZERO_PROPERTY_VALUE_VIEW_HPP

/*****************************************************************************

vtzero - Minimalistic vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <cstring>
#include <utility>

namespace vtzero {

    /**
     * A view of a vector tile property value.
     *
     * Doesn't hold any data itself, just references the value.
     */
    class property_value_view {

        data_view m_value{};

        static bool check_tag_and_type(protozero::pbf_tag_type tag, protozero::pbf_wire_type type) noexcept {
            static constexpr const protozero::pbf_wire_type types[] = {
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

            return types[tag] == type; // NOLINT clang-tidy: cppcoreguidelines-pro-bounds-constant-array-index
        }

        static data_view get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, string_value_type /* dummy */) {
            return value_message.get_view();
        }

        static float get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, float_value_type /* dummy */) {
            return value_message.get_float();
        }

        static double get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, double_value_type /* dummy */) {
            return value_message.get_double();
        }

        static int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, int_value_type /* dummy */) {
            return value_message.get_int64();
        }

        static uint64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, uint_value_type /* dummy */) {
            return value_message.get_uint64();
        }

        static int64_t get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, sint_value_type /* dummy */) {
            return value_message.get_sint64();
        }

        static bool get_value_impl(protozero::pbf_message<detail::pbf_value>& value_message, bool_value_type /* dummy */) {
            return value_message.get_bool();
        }

        template <typename T>
        typename T::type get_value() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            typename T::type result{};
            bool has_result = false;
            while (value_message.next(T::pvtype, T::wire_type)) {
                result = get_value_impl(value_message, T());
                has_result = true;
            }

            if (has_result) {
                return result;
            }

            throw type_exception{};
        }

    public:

        /**
         * The default constructor creates an invalid (empty)
         * property_value_view.
         */
        property_value_view() noexcept = default;

        /**
         * Create a (valid) property_view from a data_view.
         */
        explicit property_value_view(const data_view value) noexcept :
            m_value(value) {
        }

        /**
         * Is this a valid view? Property value views are valid if they were
         * constructed using the non-default constructor.
         */
        bool valid() const noexcept {
            return m_value.data() != nullptr;
        }

        /**
         * Get the type of this property value.
         *
         * @pre @code valid() @endcode
         * @throws format_exception if the encoding is invalid
         */
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

        /**
         * Get the internal data_view this object was constructed with.
         */
        data_view data() const noexcept {
            return m_value;
        }

        /**
         * Get string value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than string.
         */
        data_view string_value() const {
            return get_value<string_value_type>();
        }

        /**
         * Get float value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than float.
         */
        float float_value() const {
            return get_value<float_value_type>();
        }

        /**
         * Get double value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than double.
         */
        double double_value() const {
            return get_value<double_value_type>();
        }

        /**
         * Get int value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than int.
         */
        std::int64_t int_value() const {
            return get_value<int_value_type>();
        }

        /**
         * Get uint value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than uint.
         */
        std::uint64_t uint_value() const {
            return get_value<uint_value_type>();
        }

        /**
         * Get sint value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than sint.
         */
        std::int64_t sint_value() const {
            return get_value<sint_value_type>();
        }

        /**
         * Get bool value of this object.
         *
         * @pre @code valid() @endcode
         * @throws type_exception if the type of this property value is
         *                        something other than bool.
         */
        bool bool_value() const {
            return get_value<bool_value_type>();
        }

    }; // class property_value_view

    /// property_value_views are equal if they contain the same data.
    inline bool operator==(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() == rhs.data();
    }

    /// property_value_views are unequal if they do not contain the same data.
    inline bool operator!=(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() != rhs.data();
    }

    /// property_value_views are ordered in the same way as the underlying data_view
    inline bool operator<(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() < rhs.data();
    }

    /// property_value_views are ordered in the same way as the underlying data_view
    inline bool operator<=(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() <= rhs.data();
    }

    /// property_value_views are ordered in the same way as the underlying data_view
    inline bool operator>(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() > rhs.data();
    }

    /// property_value_views are ordered in the same way as the underlying data_view
    inline bool operator>=(const property_value_view& lhs, const property_value_view& rhs) noexcept {
        return lhs.data() >= rhs.data();
    }

    /**
     * Apply the value to a visitor.
     *
     * The visitor must have an overloaded call operator taking single
     * argument of types data_view, float, double, int64_t, uint64_t, and bool.
     * All call operators must return the same type which will be the return
     * type of this function.
     */
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

        }; // struct convert_visitor

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
