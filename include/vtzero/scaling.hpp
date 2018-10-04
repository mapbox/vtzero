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

        /// Construct scaling with specified values (used for testing).
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
                        m_offset = reader.get_int64();
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

    }; // class scaling

} // namespace vtzero

#endif // VTZERO_SCALING_HPP
