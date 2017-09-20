#ifndef VTZERO_PROPERTY_VALUE_HPP
#define VTZERO_PROPERTY_VALUE_HPP

#include "types.hpp"

#include <protozero/pbf_builder.hpp>

#include <string>

namespace vtzero {

    class property_value {

        std::string m_data;

    public:

        data_view data() const noexcept {
            return {m_data.data(), m_data.size()};
        }

        // ------------------

        explicit property_value(string_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value.value);
        }

        explicit property_value(const char* value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        explicit property_value(const std::string& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        explicit property_value(const data_view& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        // ------------------

        explicit property_value(float_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_float(detail::pbf_value::float_value, value.value);
        }

        explicit property_value(float value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_float(detail::pbf_value::float_value, value);
        }

        // ------------------

        explicit property_value(double_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_double(detail::pbf_value::double_value, value.value);
        }

        explicit property_value(double value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_double(detail::pbf_value::double_value, value);
        }

        // ------------------

        explicit property_value(int_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, value.value);
        }

        explicit property_value(int64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, value);
        }

        // ------------------

        explicit property_value(uint_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, value.value);
        }

        explicit property_value(uint64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, value);
        }

        // ------------------

        explicit property_value(sint_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_sint64(detail::pbf_value::sint_value, value.value);
        }

        // ------------------

        explicit property_value(bool_value_type value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_bool(detail::pbf_value::bool_value, value.value);
        }

        explicit property_value(bool value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_bool(detail::pbf_value::bool_value, value);
        }

    }; // class property_value

} // namespace vtzero

#endif // VTZERO_PROPERTY_VALUE_HPP
