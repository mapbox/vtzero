#ifndef VTZERO_PROPERTY_VIEW_HPP
#define VTZERO_PROPERTY_VIEW_HPP

#include "property_value_view.hpp"
#include "types.hpp"

namespace vtzero {

    /**
     * A view of a vector tile property.
     *
     * Doesn't hold any data itself, just references to the key and value.
     */
    class property_view {

        data_view m_key{};
        property_value_view m_value{};

    public:

        /**
         * The default constructor creates an invalid (empty) property_view.
         */
        property_view() noexcept = default;

        /**
         * Create a (valid) property_view from a key and value.
         */
        property_view(const data_view key, const property_value_view value) noexcept :
            m_key(key),
            m_value(value) {
        }

        /**
         * Is this a valid view? Property views are valid if they were
         * constructed using the non-default constructor.
         */
        bool valid() const noexcept {
            return m_key.data() != nullptr;
        }

        /**
         * Is this a valid view? Property views are valid if they were
         * constructed using the non-default constructor.
         */
        explicit operator bool() const noexcept {
            return valid();
        }

        /// Return the property key.
        data_view key() const noexcept {
            return m_key;
        }

        /// Return the property value.
        property_value_view value() const noexcept {
            return m_value;
        }

    }; // class property_view

} // namespace vtzero

#endif // VTZERO_PROPERTY_VIEW_HPP
