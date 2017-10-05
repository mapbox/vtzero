#ifndef VTZERO_VECTOR_TILE_HPP
#define VTZERO_VECTOR_TILE_HPP

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
     * Iterator for iterating over layers in a tile. You usually do not
     * create these but get them from calling vector_tile::begin()/end().
     */
    class tile_iterator {

        protozero::pbf_message<detail::pbf_tile> m_tile_reader;
        data_view m_data{};

        void next() {
            if (m_tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                m_data = m_tile_reader.get_view();
            } else {
                m_data = data_view{};
            }
        }

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = layer;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        /**
         * Construct special "end" iterator.
         */
        tile_iterator() = default;

        /**
         * Construct layer iterator from specified vector tile data.
         *
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        explicit tile_iterator(const data_view& tile_data) :
            m_tile_reader(tile_data) {
            next();
        }

        /**
         * Dereference operator to get the layer.
         *
         * @returns layer
         */
        layer operator*() const {
            assert(m_data.data() != nullptr);
            return layer{m_data};
        }

        /**
         * Prefix increment operator.
         *
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        tile_iterator& operator++() {
            next();
            return *this;
        }

        /**
         * Postfix increment operator.
         *
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        tile_iterator operator++(int) {
            const tile_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        /// Equality operator
        bool operator==(const tile_iterator& other) const noexcept {
            return m_data == other.m_data;
        }

        /// Inequality operator
        bool operator!=(const tile_iterator& other) const noexcept {
            return !(*this == other);
        }

    }; // tile_iterator

    /**
     * A vector tile is basically nothing more than an ordered collection
     * of named layers. Use the subscript operator to access a layer by index
     * or name. Use begin()/end() to iterate over the layers. Using the
     * iterators is the most efficient way if you are looking at all they
     * layers.
     */
    class vector_tile {

        data_view m_data;

    public:

        using iterator = tile_iterator;
        using const_iterator = tile_iterator;

        /**
         * Construct the vector_tile from a data_view. The vector_tile object
         * will keep a reference to the data referenced by the data_view. No
         * copy of the data is done.
         */
        explicit vector_tile(const data_view data) noexcept :
            m_data(data) {
        }

        /**
         * Construct the vector_tile from a string. The vector_tile object
         * will keep a reference to the data referenced by the string. No
         * copy of the data is done.
         */
        explicit vector_tile(const std::string& data) noexcept :
            m_data(data.data(), data.size()) {
        }

        /// Returns an iterator to the beginning of layers.
        tile_iterator begin() const {
            return tile_iterator{m_data};
        }

        /// Returns an iterator to the end of layers.
        tile_iterator end() const {
            return tile_iterator{};
        }

        /**
         * Is this vector tile empty? Returns true if there are no layers
         * in this vector tile.
         *
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
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        std::size_t size() const {
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
        layer operator[](std::size_t index) const {
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
         * Returns the layer with the specified zero-based index.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns The specified layer or the invalid layer if index is
         *          larger than the number of layers.
         * @throws format_exception if the tile data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         */
        template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
        layer operator[](T index) const {
            return operator[](std::size_t(index));
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
        layer operator[](const data_view& name) const {
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
         */
        layer operator[](const std::string& name) const {
            const data_view dv{name.data(), name.size()};
            return (*this)[dv];
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
         */
        layer operator[](const char* name) const {
            const data_view dv{name, std::strlen(name)};
            return (*this)[dv];
        }

    }; // class vector_tile

} // namespace vtzero

#endif // VTZERO_VECTOR_TILE_HPP
