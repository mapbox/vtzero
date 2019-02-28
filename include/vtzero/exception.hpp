#ifndef VTZERO_EXCEPTION_HPP
#define VTZERO_EXCEPTION_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file exception.hpp
 *
 * @brief Contains the exceptions used in the vtzero library.
 */

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace vtzero {

    /// Base class for all exceptions directly thrown by the vtzero library.
    class exception : public std::runtime_error {

        std::size_t m_layer_num;
        std::size_t m_feature_num;

    public:

        /// Constructor
        explicit exception(const char* message, std::size_t layer_num = 0, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            std::runtime_error(message),
            m_layer_num(layer_num),
            m_feature_num(feature_num) {
        }

        /// Constructor
        explicit exception(const std::string& message, std::size_t layer_num = 0, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            std::runtime_error(message),
            m_layer_num(layer_num),
            m_feature_num(feature_num) {
        }

        /**
         * Return the (zero-based) layer index in the vector tile that
         * triggered this exception.
         */
        std::size_t layer_num() const noexcept {
            return m_layer_num;
        }

        /**
         * Return the (zero-based) feature index in the layer that
         * triggered this exception.
         *
         * @pre Check with has_feature_num() whether there is actually
         *      a feature index stored in the exception.
         */
        std::size_t feature_num() const noexcept {
            return m_feature_num;
        }

        /**
         * Does this exception contain a feature number?
         */
        bool has_feature_num() const noexcept {
            return m_feature_num != std::numeric_limits<std::size_t>::max();
        }

    }; // class exception

    /**
     * @brief Error in vector tile format
     *
     * This exception is thrown when vector tile encoding isn't valid according
     * to the vector tile specification.
     */
    class format_exception : public exception {

    public:

        /// Constructor
        explicit format_exception(const char* message, std::size_t layer_num, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            exception(message, layer_num, feature_num) {
        }

        /// Constructor
        explicit format_exception(const std::string& message, std::size_t layer_num = 0, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            exception(message, layer_num, feature_num) {
        }

    }; // class format_exception

    /**
     * @brief Error in vector tile geometry
     *
     * This exception is thrown when a geometry encoding isn't valid according
     * to the vector tile specification.
     */
    class geometry_exception : public exception {

    public:

        /// Constructor
        explicit geometry_exception(const char* message, std::size_t layer_num = 0, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            exception(message, layer_num, feature_num) {
        }

        /// Constructor
        explicit geometry_exception(const std::string& message, std::size_t layer_num = 0, std::size_t feature_num = std::numeric_limits<std::size_t>::max()) :
            exception(message, layer_num, feature_num) {
        }

    }; // class geometry_exception

    /**
     * This exception is thrown when a property value is accessed using the
     * wrong type.
     */
    class type_exception : public exception {

    public:

        /// Constructor
        explicit type_exception() :
            exception("wrong property value type") {
        }

    }; // class type_exception

    /**
     * @brief Unknown version error
     *
     * This exception is thrown when an unknown version number is found in the
     * layer.
     */
    class version_exception : public exception {

    public:

        /// Constructor
        explicit version_exception(const uint32_t version, std::size_t layer_num) :
            exception(std::string{"Layer with unknown version "} +
                      std::to_string(version) + " (spec 4.1)",
                      layer_num) {
        }

    }; // version_exception

    /**
     * @brief Invalid index into layer table
     *
     * This exception is thrown when an index into the key or one of the value
     * tables in a layer is out of range. This can only happen if the tile data
     * is invalid.
     */
    class out_of_range_exception : public exception {

    public:

        /// Constructor
        explicit out_of_range_exception(const uint32_t index, std::size_t layer_num) :
            exception(std::string{"Index out of range: "} +
                      std::to_string(index),
                      layer_num) {
        }

    }; // out_of_range_exception

} // namespace vtzero

#endif // VTZERO_EXCEPTION_HPP
