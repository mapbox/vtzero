#ifndef VTZERO_VECTOR_TILE_HPP
#define VTZERO_VECTOR_TILE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file vector_tile.hpp
 *
 * @brief Contains the vector_tile class.
 */

#include "exception.hpp"
#include "layer.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

namespace vtzero {

    /**
     * For iterating over all layers in a vector tile.
     *
     * Usage:
     * @code
     * vtzero::vector_tile tile = ...;
     * for (auto layer : tile) {
     *   ...
     * }
     * @endcode
     */
    class layer_iterator {

        data_view m_data{};

        void skip_non_layers() {
            protozero::pbf_message<detail::pbf_tile> reader{m_data};
            while (reader.next()) {
                if (reader.tag_and_type() ==
                        protozero::tag_and_type(detail::pbf_tile::layers,
                                                protozero::pbf_wire_type::length_delimited)) {
                    return;
                }
                reader.skip();
                m_data = reader.data();
            }
            m_data = reader.data();
        }

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = layer;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        layer_iterator() noexcept = default;

        explicit layer_iterator(data_view data) noexcept :
            m_data(std::move(data)) {
            skip_non_layers();
        }

        value_type operator*() const {
            protozero::pbf_message<detail::pbf_tile> reader{m_data};
            if (reader.next(detail::pbf_tile::layers,
                            protozero::pbf_wire_type::length_delimited)) {
                return layer{reader.get_view()};
            }
            throw format_exception{"expected layer"};
        }

        layer_iterator& operator++() {
            if (m_data.size() > 0) {
                protozero::pbf_message<detail::pbf_tile> reader{m_data};
                if (reader.next(detail::pbf_tile::layers,
                                protozero::pbf_wire_type::length_delimited)) {
                    reader.skip();
                    m_data = reader.data();
                    skip_non_layers();
                }
            }
            return *this;
        }

        layer_iterator operator++(int) {
            const layer_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        friend bool operator==(layer_iterator lhs, layer_iterator rhs) noexcept {
            return lhs.m_data == rhs.m_data;
        }

        friend bool operator!=(layer_iterator lhs, layer_iterator rhs) noexcept {
            return !(lhs == rhs);
        }

    }; // class layer_iterator

    /**
     * A vector tile is basically nothing more than an ordered collection
     * of named layers. For the most efficient way to access the layers
     * iterate over them using a range-for:
     *
     * @code
     *   std::string data = ...;
     *   const vtzero::vector_tile tile{data};
     *   for (auto layer : tile) {
     *     ...
     *   }
     * @endcode
     *
     * If you know the index of the layer, you can get it directly with
     * @code
     *   tile.get_layer(4);
     * @endcode
     *
     * You can also access the layer by name:
     * @code
     *   tile.get_layer_by_name("foobar");
     * @endcode
     */
    class vector_tile {

        data_view m_data;

    public:

        /**
         * Construct the vector_tile from a data_view. The vector_tile object
         * will keep a reference to the data referenced by the data_view. No
         * copy of the data is created.
         */
        explicit vector_tile(const data_view data) noexcept :
            m_data(data) {
        }

        /**
         * Construct the vector_tile from a string. The vector_tile object
         * will keep a reference to the data referenced by the string. No
         * copy of the data is created.
         */
        explicit vector_tile(const std::string& data) noexcept :
            m_data(data.data(), data.size()) {
        }

        /**
         * Construct the vector_tile from a ptr and size. The vector_tile
         * object will keep a reference to the data. No copy of the data is
         * created.
         */
        vector_tile(const char* data, std::size_t size) noexcept :
            m_data(data, size) {
        }

        /**
         * Is this vector tile empty?
         *
         * @returns true if there are no layers in this vector tile, false
         *          otherwise
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_data.empty();
        }

        /**
         * Return the number of layers in this tile.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns the number of layers in this tile
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        std::size_t count_layers() const {
            std::size_t size = 0;

            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};
            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                tile_reader.skip();
                ++size;
            }

            return size;
        }

        /**
         * Returns the layer with the specified zero-based index.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns The specified layer or the invalid layer if index is
         *          larger than the number of layers.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer(std::size_t index) const {
            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};

            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                if (index == 0) {
                    return layer{tile_reader.get_view()};
                }
                tile_reader.skip();
                --index;
            }

            return layer{};
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const data_view name) const {
            protozero::pbf_message<detail::pbf_tile> tile_reader{m_data};

            while (tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                const auto layer_data = tile_reader.get_view();
                protozero::pbf_message<detail::pbf_layer> layer_reader{layer_data};
                if (layer_reader.next(detail::pbf_layer::name,
                                      protozero::pbf_wire_type::length_delimited)) {
                    if (layer_reader.get_view() == name) {
                        return layer{layer_data};
                    }
                } else {
                    // 4.1 "A layer MUST contain a name field."
                    throw format_exception{"missing name in layer (spec 4.1)"};
                }
            }

            return layer{};
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const std::string& name) const {
            return get_layer_by_name(data_view{name.data(), name.size()});
        }

        /**
         * Returns the layer with the specified name.
         *
         * Complexity: Linear in the number of layers.
         *
         * If there are several layers with the same name (which is against
         * the spec 4.1 "A Vector Tile MUST NOT contain two or more layers
         * whose name values are byte-for-byte identical.") it is unspecified
         * which will be returned.
         *
         * @returns The specified layer or the invalid layer if there is no
         *          layer with this name.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        layer get_layer_by_name(const char* name) const {
            return get_layer_by_name(data_view{name, std::strlen(name)});
        }

        /// Get a (const) iterator to the first layer in this vector tile.
        layer_iterator begin() const noexcept {
            return layer_iterator{m_data};
        }

        /// Get a (const) iterator one past the end layer in this vector tile.
        layer_iterator end() const noexcept {
            return layer_iterator{data_view{m_data.data() + m_data.size(), 0}};
        }

    }; // class vector_tile

    /**
     * Helper function to determine whether some data could represent a
     * vector tile. This takes advantage of the fact that the first byte of
     * a vector tile is always 0x1a. It can't be 100% reliable though, because
     * some other data could still contain that byte.
     *
     * @returns false if this is definitely no vector tile
     *          true if this could be a vector tile
     */
    inline bool is_vector_tile(const data_view data) noexcept {
        return !data.empty() && data.data()[0] == 0x1a;
    }

} // namespace vtzero

#endif // VTZERO_VECTOR_TILE_HPP
