#ifndef VTZERO_TILE_BUILDER_HPP
#define VTZERO_TILE_BUILDER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file tile_builder.hpp
 *
 * @brief Contains the tile_builder class.
 */

#include "detail/builder_impl.hpp"
#include "layer.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace vtzero {

    /**
     * Used to build vector tiles. Whenever you are building a new vector
     * tile, start with an object of this class and add layers. After all
     * the data is added, call serialize().
     *
     * @code
     * layer some_existing_layer = ...;
     *
     * tile_builder builder;
     * layer_builder layer_roads{builder, "roads"};
     * builder.add_existing_layer(some_existing_layer);
     * ...
     * std::string data = builder.serialize();
     * @endcode
     */
    class tile_builder {

        friend class layer_builder;

        std::vector<std::unique_ptr<detail::layer_builder_base>> m_layers;

        /**
         * Add a new layer to the vector tile based on an existing layer. The
         * new layer will have the same name, version, and extent as the
         * existing layer. The new layer will not contain any features. This
         * method is handy when copying some (but not all) data from an
         * existing layer.
         */
        detail::layer_builder_impl* add_layer(const layer& layer) {
            const auto ptr = layer.get_tile().valid() ? new detail::layer_builder_impl{layer.name(), layer.version(), layer.get_tile()}
                                                      : new detail::layer_builder_impl{layer.name(), layer.version(), layer.extent()};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        /**
         * Add a new layer to the vector tile with the specified name, version,
         * and extent.
         *
         * @tparam TString Some string type (const char*, std::string,
         *         vtzero::data_view) or something that converts to one of
         *         these types.
         * @param name Name of this layer.
         * @param version Version of this layer (only version 1 and 2 are
         *                supported)
         * @param extent Extent used for this layer.
         */
        template <typename TString>
        detail::layer_builder_impl* add_layer(TString&& name, uint32_t version, uint32_t extent) {
            const auto ptr = new detail::layer_builder_impl{std::forward<TString>(name), version, extent};
            m_layers.emplace_back(ptr);
            return ptr;
        }

        /**
         * Add a new layer to the vector tile with the specified name, version,
         * and extent.
         *
         * @tparam TString Some string type (const char*, std::string,
         *         vtzero::data_view) or something that converts to one of
         *         these types.
         * @param name Name of this layer.
         * @param version Version of this layer (only version 1 and 2 are
         *                supported)
         * @param tile The tile (x, y, zoom, extent) of the new layer.
         */
        template <typename TString>
        detail::layer_builder_impl* add_layer(TString&& name, uint32_t version, const tile& tile) {
            const auto ptr = new detail::layer_builder_impl{std::forward<TString>(name), version, tile};
            m_layers.emplace_back(ptr);
            return ptr;
        }

    public:

        /// Constructor
        tile_builder() = default;

        /// Destructor
        ~tile_builder() noexcept = default;

        /// Tile builders can not be copied.
        tile_builder(const tile_builder&) = delete;

        /// Tile builders can not be copied.
        tile_builder& operator=(const tile_builder&) = delete;

        /// Tile builders can be moved.
        tile_builder(tile_builder&&) = default;

        /// Tile builders can be moved.
        tile_builder& operator=(tile_builder&&) = default;

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param data Reference to some data that must be a valid encoded
         *        layer.
         */
        void add_existing_layer(data_view&& data) {
            m_layers.emplace_back(new detail::layer_builder_existing{std::forward<data_view>(data)});
        }

        /**
         * Add an existing layer to the vector tile. The layer data will be
         * copied over into the new vector_tile when the serialize() method
         * is called. Until then, the data referenced here must stay available.
         *
         * @param layer Reference to the layer to be copied.
         */
        void add_existing_layer(const layer& layer) {
            add_existing_layer(layer.data());
        }

        /**
         * Serialize the data accumulated in this builder into a vector tile.
         * The data will be appended to the specified buffer. The buffer
         * doesn't have to be empty.
         *
         * @param buffer Buffer to append the encoded vector tile to.
         */
        void serialize(std::string& buffer) const {
            std::size_t estimated_size = 0;
            for (const auto& layer : m_layers) {
                estimated_size += layer->estimated_size();
            }

            buffer.reserve(buffer.size() + estimated_size);

            protozero::pbf_builder<detail::pbf_tile> pbf_tile_builder{buffer};
            for (const auto& layer : m_layers) {
                layer->build(pbf_tile_builder);
            }
        }

        /**
         * Serialize the data accumulated in this builder into a vector_tile
         * and return it.
         *
         * If you want to use an existing buffer instead, use the serialize()
         * method taking a std::string& as parameter.
         *
         * @returns std::string Buffer with encoded vector_tile data.
         */
        std::string serialize() const {
            std::string data;
            serialize(data);
            return data;
        }

    }; // class tile_builder

} // namespace vtzero

#endif // VTZERO_TILE_BUILDER_HPP
