#ifndef VTZERO_READER_HPP
#define VTZERO_READER_HPP

#include "exception.hpp"
#include "geometry.hpp"
#include "types.hpp"

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vtzero {

    class tag_value {

        std::string m_data;

    public:

        data_view data() const noexcept {
            return {m_data.data(), m_data.size()};
        }

        explicit tag_value(const char* value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        explicit tag_value(const std::string& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        explicit tag_value(const data_view& value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_string(detail::pbf_value::string_value, value);
        }

        explicit tag_value(float value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_float(detail::pbf_value::float_value, value);
        }

        explicit tag_value(double value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_double(detail::pbf_value::double_value, value);
        }

        explicit tag_value(int64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_int64(detail::pbf_value::int_value, value);
        }

        explicit tag_value(uint64_t value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_uint64(detail::pbf_value::uint_value, value);
        }

        explicit tag_value(int64_t value, int /*unused*/) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_sint64(detail::pbf_value::sint_value, value);
        }

        explicit tag_value(bool value) {
            protozero::pbf_builder<detail::pbf_value> pbf_message_value{m_data};
            pbf_message_value.add_bool(detail::pbf_value::bool_value, value);
        }

    }; // class tag_value

    inline tag_value float_value(float value) {
        return tag_value{value};
    }

    inline tag_value double_value(double value) {
        return tag_value{value};
    }

    inline tag_value int_value(int64_t value) {
        return tag_value{value};
    }

    inline tag_value uint_value(uint64_t value) {
        return tag_value{value};
    }

    inline tag_value sint_value(int64_t value) {
        return tag_value{value, 0};
    }

    inline tag_value bool_value(bool value) {
        return tag_value{value};
    }

    class value_view {

        data_view m_value;

        protozero::pbf_message<detail::pbf_value> check_value(detail::pbf_value type, protozero::pbf_wire_type wire_type) const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};

            if (!value_message.next(type, wire_type)) {
                throw type_exception{};
            }

            return value_message;
        }

    public:

        value_view() = default;

        value_view(const data_view& value) noexcept :
            m_value(value) {
        }

        bool valid() const noexcept {
            return m_value.data() != nullptr;
        }

        value_type type() const {
            assert(valid());
            protozero::pbf_message<detail::pbf_value> value_message{m_value};
            if (value_message.next()) {
                return value_message.tag();
            }
            throw format_exception{"missing tag value"};
        }

        data_view data() const noexcept {
            return m_value;
        }

        data_view string_value() const {
            return check_value(detail::pbf_value::string_value, protozero::pbf_wire_type::length_delimited).get_view();
        }

        float float_value() const {
            return check_value(detail::pbf_value::float_value, protozero::pbf_wire_type::fixed32).get_float();
        }

        double double_value() const {
            return check_value(detail::pbf_value::double_value, protozero::pbf_wire_type::fixed64).get_double();
        }

        std::int64_t int_value() const {
            return check_value(detail::pbf_value::int_value, protozero::pbf_wire_type::varint).get_int64();
        }

        std::uint64_t uint_value() const {
            return check_value(detail::pbf_value::uint_value, protozero::pbf_wire_type::varint).get_uint64();
        }

        std::int64_t sint_value() const {
            return check_value(detail::pbf_value::sint_value, protozero::pbf_wire_type::varint).get_sint64();
        }

        bool bool_value() const {
            return check_value(detail::pbf_value::bool_value, protozero::pbf_wire_type::varint).get_bool();
        }

    }; // class value_view

    template <typename V>
    void value_visit(V&& visitor, const value_view& value) {
        switch (value.type()) {
            case detail::pbf_value::string_value:
                std::forward<V>(visitor)(value.string_value());
                break;
            case detail::pbf_value::float_value:
                std::forward<V>(visitor)(value.float_value());
                break;
            case detail::pbf_value::double_value:
                std::forward<V>(visitor)(value.double_value());
                break;
            case detail::pbf_value::int_value:
                std::forward<V>(visitor)(value.int_value());
                break;
            case detail::pbf_value::uint_value:
                std::forward<V>(visitor)(value.uint_value());
                break;
            case detail::pbf_value::sint_value:
                std::forward<V>(visitor)(value.sint_value());
                break;
            case detail::pbf_value::bool_value:
                std::forward<V>(visitor)(value.bool_value());
                break;
        }
    }

    /**
     * A vector tile tag.
     */
    class tag_view {

        data_view m_key;
        value_view m_value;

    public:

        tag_view() = default;

        tag_view(const data_view& key, const data_view& value) noexcept :
            m_key(key),
            m_value(value) {
        }

        bool valid() const noexcept {
            return m_key.data() != nullptr;
        }

        operator bool() const noexcept {
            return valid();
        }

        data_view key() const noexcept {
            return m_key;
        }

        value_view value() const noexcept {
            return m_value;
        }

    }; // class tag_view

    class layer;

    class tags_iterator {

        protozero::pbf_reader::const_uint32_iterator m_it;
        protozero::pbf_reader::const_uint32_iterator m_end;
        const layer* m_layer;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = tag_view;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        tags_iterator(const protozero::pbf_reader::const_uint32_iterator& begin,
                      const protozero::pbf_reader::const_uint32_iterator& end,
                      const layer* layer) :
            m_it(begin),
            m_end(end),
            m_layer(layer) {
            assert(layer);
        }

        tag_view operator*() const;

        tags_iterator& operator++() {
            ++m_it;
            if (m_it == m_end) {
                throw format_exception{"unpaired tag key/value indexes (spec 4.4)"};
            }
            ++m_it;
            return *this;
        }

        tags_iterator operator++(int) {
            const tags_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const tags_iterator& other) const noexcept {
            return m_it == other.m_it &&
                   m_end == other.m_end &&
                   m_layer == other.m_layer;
        }

        bool operator!=(const tags_iterator& other) const noexcept {
            return !(*this == other);
        }

        /**
         * Returns true if there are no tags.
         *
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_it == m_end;
        }

        /**
         * Return the number of tags.
         *
         * Complexity: Linear.
         */
        std::size_t size() const noexcept {
            return m_it.size() / 2;
        }

    }; // tags_iterator

    /**
     * A feature according to spec 4.2.
     */
    class feature {

        using uint32_it_range = protozero::iterator_range<protozero::pbf_reader::const_uint32_iterator>;

        uint64_t m_id;
        uint32_it_range m_tags;
        data_view m_geometry;
        GeomType m_type;
        bool m_has_id;

    public:

        feature() :
            m_id(0),
            m_tags(),
            m_geometry(),
            m_type(),
            m_has_id(false) {
        }

        feature(const data_view& data) :
            m_id(0), // defaults to 0, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L32
            m_tags(),
            m_geometry(),
            m_type(), // defaults to UNKNOWN, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L41
            m_has_id(false) {

            protozero::pbf_message<detail::pbf_feature> reader{data};

            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_feature::id, protozero::pbf_wire_type::varint):
                        m_id = reader.get_uint64();
                        m_has_id = true;
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::tags, protozero::pbf_wire_type::length_delimited):
                        m_tags = reader.get_packed_uint32();
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::type, protozero::pbf_wire_type::varint): {
                            const auto type = reader.get_enum();
                            // spec 4.3.4 "Geometry Types"
                            if (type < 0 || type > 3) {
                                throw format_exception{"Unknown geometry type (spec 4.3.4)"};
                            }
                            m_type = static_cast<GeomType>(type);
                        }
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::geometry, protozero::pbf_wire_type::length_delimited):
                        m_geometry = reader.get_view();
                        break;
                    default:
                        reader.skip();
                }
            }

            // spec 4.2 "A feature MUST contain a geometry field."
            if (m_geometry.empty()) {
                throw format_exception{"Missing geometry field in feature (spec 4.2)"};
            }
        }

        bool valid() const noexcept {
            return m_geometry.data() != nullptr;
        }

        operator bool() const noexcept {
            return valid();
        }

        uint64_t id() const noexcept {
            assert(valid());
            return m_id;
        }

        bool has_id() const noexcept {
            return m_has_id;
        }

        GeomType type() const noexcept {
            assert(valid());
            return m_type;
        }

        const data_view& geometry() const noexcept {
            return m_geometry;
        }

        protozero::iterator_range<tags_iterator> tags(const layer& layer) const noexcept {
            return {{m_tags.begin(), m_tags.end(), &layer},
                    {m_tags.end(), m_tags.end(), &layer}};
        }

    }; // class feature

    class feature_iterator {

        protozero::pbf_message<detail::pbf_layer> m_layer_reader;
        data_view m_data;

        void next() {
            try {
                if (m_layer_reader.next(detail::pbf_layer::features,
                                        protozero::pbf_wire_type::length_delimited)) {
                    m_data = m_layer_reader.get_view();
                } else {
                    m_data = data_view{};
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }
        }

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = feature;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        /**
         * Construct special "end" iterator.
         */
        feature_iterator() = default;

        /**
         * Construct feature iterator from specified vector tile data.
         *
         * @throws format_exception if the tile data is ill-formed.
         */
        feature_iterator(const data_view& tile_data) :
            m_layer_reader(tile_data),
            m_data() {
            next();
        }

        feature operator*() const {
            assert(m_data.data() != nullptr);
            return feature{m_data};
        }

        feature operator->() const {
            assert(m_data.data() != nullptr);
            return feature{m_data};
        }

        /**
         * @throws format_exception if the layer data is ill-formed.
         */
        feature_iterator& operator++() {
            next();
            return *this;
        }

        /**
         * @throws format_exception if the layer data is ill-formed.
         */
        feature_iterator operator++(int) {
            const feature_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const feature_iterator& other) const noexcept {
            return m_data == other.m_data;
        }

        bool operator!=(const feature_iterator& other) const noexcept {
            return !(*this == other);
        }

    }; // feature_iterator

    /**
     * A layer according to spec 4.1
     */
    class layer {

        data_view m_data;
        uint32_t m_version;
        uint32_t m_extent;
        data_view m_name;
        std::vector<data_view> m_key_table;
        std::vector<data_view> m_value_table;

    public:

        layer() :
            m_data(),
            m_version(0),
            m_extent(0),
            m_name() {
        }

        /**
         * Construct a layer object.
         *
         * @throws format_exception if the layer data is ill-formed.
         * @throws version_exception if the layer contains an unsupported version
         *                           number (only version 1 and 2 are supported)
         */
        explicit layer(const data_view& data) :
            m_data(data),
            m_version(1), // defaults to 1, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L55
            m_extent(4096), // defaults to 4096, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L70
            m_name() {
            try {
                protozero::pbf_message<detail::pbf_layer> reader{data};
                while (reader.next()) {
                    switch (reader.tag_and_type()) {
                        case protozero::tag_and_type(detail::pbf_layer::version, protozero::pbf_wire_type::varint):
                            m_version = reader.get_uint32();
                            // This library can only handle version 1 and 2.
                            if (m_version < 1 || m_version > 2) {
                                throw version_exception{m_version};
                            }
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::name, protozero::pbf_wire_type::length_delimited):
                            m_name = reader.get_view();
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::keys, protozero::pbf_wire_type::length_delimited):
                            m_key_table.push_back(reader.get_view());
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::values, protozero::pbf_wire_type::length_delimited):
                            m_value_table.push_back(reader.get_view());
                            break;
                        case protozero::tag_and_type(detail::pbf_layer::extent, protozero::pbf_wire_type::varint):
                            m_extent = reader.get_uint32();
                            break;
                        default:
                            reader.skip();
                            break;
                    }
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            // 4.1 "A layer MUST contain a name field."
            if (m_name.data() == nullptr) {
                throw format_exception{"missing name field in layer (spec 4.1)"};
            }
        }

        bool valid() const noexcept {
            return m_version != 0;
        }

        operator bool() const noexcept {
            return valid();
        }

        data_view data() const noexcept {
            return m_data;
        }

        data_view name() const noexcept {
            assert(valid());
            return m_name;
        }

        std::uint32_t version() const noexcept {
            assert(valid());
            return m_version;
        }

        std::uint32_t extent() const noexcept {
            assert(valid());
            return m_extent;
        }

        const std::vector<data_view>& key_table() const noexcept {
            return m_key_table;
        }

        const std::vector<data_view>& value_table() const noexcept {
            return m_value_table;
        }

        const data_view& key(uint32_t n) const {
            if (n >= m_key_table.size()) {
                throw format_exception{std::string{"key table index too large: "} + std::to_string(n)};
            }

            return m_key_table[n];
        }

        const data_view& value(uint32_t n) const {
            if (n >= m_value_table.size()) {
                throw format_exception{std::string{"value table index too large: "} + std::to_string(n)};
            }

            return m_value_table[n];
        }

        feature_iterator begin() const {
            return feature_iterator{m_data};
        }

        feature_iterator end() const {
            return feature_iterator{};
        }

        /**
         * Get the feature with the specified ID.
         *
         * Complexity: Linear in the number of features.
         *
         * @param id The ID to look for.
         * @returns Feature with the specified ID or the invalid feature if
         *          there is no feature with this ID.
         * @throws format_exception if the layer data is ill-formed.
         */
        feature get_feature(uint64_t id) const {
            assert(valid());

            try {
                protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
                while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                    const auto feature_data = layer_reader.get_view();
                    protozero::pbf_message<detail::pbf_feature> feature_reader{feature_data};
                    if (feature_reader.next(detail::pbf_feature::id, protozero::pbf_wire_type::varint)) {
                        if (feature_reader.get_uint64() == id) {
                            return feature{feature_data};
                        }
                    }
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            return feature{};
        }

        /**
         * Count the number of features in this layer.
         *
         * Complexity: Linear in the number of features.
         *
         * @throws format_exception if the layer data is ill-formed.
         */
        std::size_t get_feature_count() const {
            assert(valid());
            std::size_t count = 0;

            try {
                protozero::pbf_message<detail::pbf_layer> layer_reader{m_data};
                while (layer_reader.next(detail::pbf_layer::features, protozero::pbf_wire_type::length_delimited)) {
                    layer_reader.skip();
                    ++count;
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
            }

            return count;
        }

    }; // class layer

    class layer_iterator {

        protozero::pbf_message<detail::pbf_tile> m_tile_reader;
        data_view m_data;

        void next() {
            try {
                if (m_tile_reader.next(detail::pbf_tile::layers,
                                    protozero::pbf_wire_type::length_delimited)) {
                    m_data = m_tile_reader.get_view();
                } else {
                    m_data = data_view{};
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
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
        layer_iterator() = default;

        /**
         * Construct layer iterator from specified vector tile data.
         *
         * @throws format_exception if the tile data is ill-formed.
         */
        layer_iterator(const data_view& tile_data) :
            m_tile_reader(tile_data),
            m_data() {
            next();
        }

        layer operator*() const {
            assert(m_data.data() != nullptr);
            return layer{m_data};
        }

        layer operator->() const {
            assert(m_data.data() != nullptr);
            return layer{m_data};
        }

        /**
         * @throws format_exception if the tile data is ill-formed.
         */
        layer_iterator& operator++() {
            next();
            return *this;
        }

        /**
         * @throws format_exception if the tile data is ill-formed.
         */
        layer_iterator operator++(int) {
            const layer_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const layer_iterator& other) const noexcept {
            return m_data == other.m_data;
        }

        bool operator!=(const layer_iterator& other) const noexcept {
            return !(*this == other);
        }

    }; // layer_iterator

    /**
     * A vector tile is basically nothing more than an ordered collection
     * of named layers. Use the subscript operator to access a layer by index
     * or name. Use begin()/end() to iterator over the layers.
     */
    class vector_tile {

        data_view m_data;

    public:

        explicit vector_tile(const data_view& data) noexcept :
            m_data(data) {
        }

        explicit vector_tile(const std::string& data) noexcept :
            m_data(data.data(), data.size()) {
        }

        /// Returns an iterator to the beginning.
        layer_iterator begin() const {
            return layer_iterator{m_data};
        }

        /// Returns an iterator to the end.
        layer_iterator end() const {
            return layer_iterator{};
        }

        /**
         * Returns the layer with the specified zero-based index.
         *
         * Complexity: Linear in the number of layers.
         *
         * @returns The specified layer or the invalid layer if index is
         *          larger than the number of layers.
         * @throws format_exception if the tile data is ill-formed.
         */
        layer operator[](std::size_t index) const {
            protozero::pbf_message<detail::pbf_tile> reader{m_data};

            try {
                while (reader.next(detail::pbf_tile::layers, protozero::pbf_wire_type::length_delimited)) {
                    if (index == 0) {
                        return layer{reader.get_view()};
                    }
                    reader.skip();
                    --index;
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
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
        layer operator[](const data_view& name) const {
            protozero::pbf_message<detail::pbf_tile> reader{m_data};

            try {
                while (reader.next(detail::pbf_tile::layers, protozero::pbf_wire_type::length_delimited)) {
                    const auto layer_data = reader.get_view();
                    protozero::pbf_message<detail::pbf_layer> layer_reader{layer_data};
                    if (layer_reader.next(detail::pbf_layer::name, protozero::pbf_wire_type::length_delimited)) {
                        if (layer_reader.get_view() == name) {
                            return layer{layer_data};
                        }
                    } else {
                        // 4.1 "A layer MUST contain a name field."
                        throw format_exception{"missing name in layer (spec 4.1)"};
                    }
                }
            } catch (const protozero::exception&) {
                // convert protozero exceptions into vtzero exception
                throw protocol_buffers_exception{};
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

    inline tag_view tags_iterator::operator*() const {
        assert(m_it != m_end);
        if (std::next(m_it) == m_end) {
            throw format_exception{"unpaired tag key/value indexes (spec 4.4)"};
        }
        return {m_layer->key(*m_it), m_layer->value(*std::next(m_it))};
    }

} // namespace vtzero

#endif // VTZERO_READER_HPP
