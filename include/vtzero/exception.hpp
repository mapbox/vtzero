#ifndef VTZERO_EXCEPTION_HPP
#define VTZERO_EXCEPTION_HPP

/**
 * @file exception.hpp
 *
 * @brief Contains the exceptions used in the vtzero library.
 */

#include <stdexcept>
#include <string>

namespace vtzero {

/**
 * All exceptions explicitly thrown by the functions of the vtzero library
 * derive from this exception.
 */
class exception : public std::runtime_error {

public:

    explicit exception(const char* message) :
        std::runtime_error(message) {
    }

    explicit exception(const std::string& message) :
        std::runtime_error(message) {
    }

}; // class exception

/**
 * This exception is thrown when vector tile encoding isn't valid according to
 * the vector tile specification.
 */
class format_exception : public exception {

public:

    explicit format_exception(const char* message) :
        exception(message) {
    }

    explicit format_exception(const std::string& message) :
        exception(message) {
    }

}; // class format_exception

/**
 * This exception is thrown when the low-level vector tile encoding isn't
 * valid according to the protocol buffer specification.
 */
class protocol_buffers_exception : public format_exception {

public:

    explicit protocol_buffers_exception() :
        format_exception("invalid protocol buffers format") {
    }

}; // class protocol_buffers_exception

/**
 * This exception is thrown when a geometry encoding isn't valid according to
 * the vector tile specification.
 */
class geometry_exception : public format_exception {

public:

    explicit geometry_exception(const char* message) :
        format_exception(message) {
    }

    explicit geometry_exception(const std::string& message) :
        format_exception(message) {
    }

}; // class geometry_exception

/**
 * This exception is thrown when a tag value is accessed using the wrong type.
 */
class type_exception : public exception {

public:

    explicit type_exception() :
        exception("wrong tag value type") {
    }

}; // class type_exception

/**
 * This exception is thrown when an unknown version number is found in the
 * layer.
 */
class version_exception : public exception {

public:

    explicit version_exception(uint32_t version) :
        exception(std::string{"unknown vector tile version "} + std::to_string(version)) {
    }

}; // version_exception

} // namespace vtzero

#endif // VTZERO_EXCEPTION_HPP
