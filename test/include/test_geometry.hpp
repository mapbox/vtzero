#ifndef TEST_GEOMETRY_HPP
#define TEST_GEOMETRY_HPP

#include <vtzero/detail/geometry.hpp>

#include <cstdint>
#include <vector>

using geom_container = std::vector<uint32_t>;
using geom_iterator = geom_container::const_iterator;

using vtzero::detail::command_move_to; // NOLINT(google-global-names-in-headers)
using vtzero::detail::command_line_to; // NOLINT(google-global-names-in-headers)
using vtzero::detail::command_close_path; // NOLINT(google-global-names-in-headers)

#endif // TEST_GEOMETRY_HPP
