#ifndef VTZERO_GEOMETRY_BASICS_HPP
#define VTZERO_GEOMETRY_BASICS_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file geometry_basics.hpp
 *
 * @brief Contains some basic classes and functions related to geometry handling.
 */

#include <cstdint>
#include <limits>

namespace vtzero {

    /**
     * Type of a polygon ring. This can either be "outer", "inner", or
     * "invalid". Invalid is used when the area of the ring is 0.
     */
    enum class ring_type {
        outer = 0,
        inner = 1,
        invalid = 2
    }; // enum class ring_type

    namespace detail {

        /// The command id type as specified in the vector tile spec
        enum class CommandId : uint32_t {
            MOVE_TO = 1,
            LINE_TO = 2,
            CLOSE_PATH = 7
        };

        inline constexpr uint32_t command_integer(CommandId id, const uint32_t count) noexcept {
            return (static_cast<uint32_t>(id) & 0x7u) | (count << 3u);
        }

        inline constexpr uint32_t command_move_to(const uint32_t count) noexcept {
            return command_integer(CommandId::MOVE_TO, count);
        }

        inline constexpr uint32_t command_line_to(const uint32_t count) noexcept {
            return command_integer(CommandId::LINE_TO, count);
        }

        inline constexpr uint32_t command_close_path() noexcept {
            return command_integer(CommandId::CLOSE_PATH, 1);
        }

        inline constexpr uint32_t get_command_id(const uint32_t command_integer) noexcept {
            return command_integer & 0x7u;
        }

        inline constexpr uint32_t get_command_count(const uint32_t command_integer) noexcept {
            return command_integer >> 3u;
        }

        // The maximum value for the command count according to the spec.
        inline constexpr uint32_t max_command_count() noexcept {
            return get_command_count(std::numeric_limits<uint32_t>::max());
        }

    } // namespace detail

} // namespace vtzero

#endif // VTZERO_GEOMETRY_BASICS_HPP
