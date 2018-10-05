#ifndef VTZERO_SCALING_HPP
#define VTZERO_SCALING_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file scaling.hpp
 *
 * @brief Contains the scaling class.
 */

#include "exception.hpp"
#include "types.hpp"

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <vector>

namespace vtzero {

    /**
     * A scaling according to spec 4.4.2.5
     */
    class scaling {

        int64_t m_offset = 0;

        double m_multiplier = 1.0;

        double m_base = 0.0;

    public:

        /// Construct scaling with default values.
        constexpr scaling() noexcept = default;

        /// Construct scaling with specified values.
        constexpr scaling(int64_t offset, double multiplier, double base) noexcept :
            m_offset(offset),
            m_multiplier(multiplier),
            m_base(base) {
        }

        /// Construct scaling from pbf message.
        explicit scaling(data_view message) {
            protozero::pbf_message<detail::pbf_scaling> reader{message};
            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_scaling::offset, protozero::pbf_wire_type::varint):
                        m_offset = reader.get_sint64();
                        break;
                    case protozero::tag_and_type(detail::pbf_scaling::multiplier, protozero::pbf_wire_type::fixed64):
                        m_multiplier = reader.get_double();
                        break;
                    case protozero::tag_and_type(detail::pbf_scaling::base, protozero::pbf_wire_type::fixed64):
                        m_base = reader.get_double();
                        break;
                    default:
                        reader.skip();
                }
            }
        }

        /// Encode a value according to the scaling.
        constexpr int64_t encode(double value) const noexcept {
            return static_cast<int64_t>((value - m_base) / m_multiplier) - m_offset;
        }

        /// Decode a value according to the scaling.
        constexpr double decode(int64_t value) const noexcept {
            return m_base + m_multiplier * (static_cast<double>(value + m_offset));
        }

        /// Has this scaling the default values?
        bool is_default() const noexcept {
            return m_offset == 0 && m_multiplier == 1.0 && m_base == 0.0;
        }

        /// Serialize scaling into pbf message.
        void serialize(detail::pbf_layer pbf_type, std::string& layer_data) const {
            std::string data;
            {
                protozero::pbf_builder<detail::pbf_scaling> pbf_scaling{data};

                if (m_offset != 0) {
                    pbf_scaling.add_sint64(detail::pbf_scaling::offset, m_offset);
                }
                if (m_multiplier != 1.0) {
                    pbf_scaling.add_double(detail::pbf_scaling::multiplier, m_multiplier);
                }
                if (m_base != 0.0) {
                    pbf_scaling.add_double(detail::pbf_scaling::base, m_base);
                }
            }

            protozero::pbf_builder<detail::pbf_layer> pbf_layer{layer_data};
            pbf_layer.add_message(pbf_type, data);
        }

        /// Scalings are the same if all values are the same.
        bool operator==(const scaling& other) const noexcept {
            return m_offset == other.m_offset &&
                   m_multiplier == other.m_multiplier &&
                   m_base == other.m_base;
        }

        /// Scalings are the different if they are not the same.
        bool operator!=(const scaling& other) const noexcept {
            return !(*this == other);
        }

    }; // class scaling

} // namespace vtzero

#endif // VTZERO_SCALING_HPP
