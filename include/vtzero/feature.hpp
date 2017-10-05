#ifndef VTZERO_FEATURE_HPP
#define VTZERO_FEATURE_HPP

#include "exception.hpp"
#include "property_value_view.hpp"
#include "property_view.hpp"
#include "types.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

namespace vtzero {

    class layer;

    class properties_iterator {

        protozero::pbf_reader::const_uint32_iterator m_it;
        const layer* m_layer;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = property_view;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        properties_iterator(const protozero::pbf_reader::const_uint32_iterator begin,
                            const layer* layer) :
            m_it(std::move(begin)),
            m_layer(layer) {
            assert(layer);
        }

        property_view operator*() const;

        properties_iterator& operator++() {
            ++m_it;
            ++m_it;
            return *this;
        }

        properties_iterator operator++(int) {
            const properties_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        bool operator==(const properties_iterator& other) const noexcept {
            return m_it == other.m_it &&
                   m_layer == other.m_layer;
        }

        bool operator!=(const properties_iterator& other) const noexcept {
            return !(*this == other);
        }

        std::pair<index_value, index_value> get_index_pair() const noexcept {
            return {*m_it, *std::next(m_it)};
        }

    }; // properties_iterator

    /**
     * A feature according to spec 4.2.
     */
    class feature {

        using uint32_it_range = protozero::iterator_range<protozero::pbf_reader::const_uint32_iterator>;

        const layer* m_layer = nullptr;
        uint64_t m_id = 0; // defaults to 0, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L32
        uint32_it_range m_properties{};
        std::size_t m_properties_size = 0;
        data_view m_geometry{};
        GeomType m_geometry_type = GeomType::UNKNOWN; // defaults to UNKNOWN, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L41
        bool m_has_id = false;

    public:

        /**
         * Construct an invalid feature object.
         */
        feature() = default;

        /**
         * Construct a feature object. This is usually not something done in
         * user code, because features are created by the layer_iterator.
         *
         * @throws format_exception if the layer data is ill-formed.
         */
        feature(const layer* layer, const data_view& data) :
            m_layer(layer) {
            assert(layer);
            assert(data.data());

            protozero::pbf_message<detail::pbf_feature> reader{data};

            while (reader.next()) {
                switch (reader.tag_and_type()) {
                    case protozero::tag_and_type(detail::pbf_feature::id, protozero::pbf_wire_type::varint):
                        m_id = reader.get_uint64();
                        m_has_id = true;
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::tags, protozero::pbf_wire_type::length_delimited):
                        if (m_properties.begin() != protozero::pbf_reader::const_uint32_iterator{}) {
                            throw format_exception{"Feature has more than one tags field"};
                        }
                        m_properties = reader.get_packed_uint32();
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::type, protozero::pbf_wire_type::varint): {
                            const auto type = reader.get_enum();
                            // spec 4.3.4 "Geometry Types"
                            if (type < 0 || type > 3) {
                                throw format_exception{"Unknown geometry type (spec 4.3.4)"};
                            }
                            m_geometry_type = static_cast<GeomType>(type);
                        }
                        break;
                    case protozero::tag_and_type(detail::pbf_feature::geometry, protozero::pbf_wire_type::length_delimited):
                        if (!m_geometry.empty()) {
                            throw format_exception{"Feature has more than one geometry field"};
                        }
                        m_geometry = reader.get_view();
                        break;
                    default:
                        throw format_exception{"unknown field in feature (tag=" +
                                               std::to_string(static_cast<uint32_t>(reader.tag())) +
                                               ", type=" +
                                               std::to_string(static_cast<uint32_t>(reader.wire_type())) +
                                               ")"};
                }
            }

            // spec 4.2 "A feature MUST contain a geometry field."
            if (m_geometry.empty()) {
                throw format_exception{"Missing geometry field in feature (spec 4.2)"};
            }

            const auto size = m_properties.size();
            if (size % 2 != 0) {
                throw format_exception{"unpaired property key/value indexes (spec 4.4)"};
            }
            m_properties_size = size / 2;
        }

        /**
         * Is this a valid feature? Valid features are those not created from
         * the default constructor.
         */
        bool valid() const noexcept {
            return m_geometry.data() != nullptr;
        }

        /**
         * Is this a valid feature? Valid features are those not created from
         * the default constructor.
         */
        explicit operator bool() const noexcept {
            return valid();
        }

        /**
         * The ID of this feature. According to the spec IDs should be unique
         * in a layer if they are set (spec 4.2).
         */
        uint64_t id() const noexcept {
            assert(valid());
            return m_id;
        }

        /**
         * Does this feature have an ID?
         */
        bool has_id() const noexcept {
            return m_has_id;
        }

        /**
         * The geometry type of this feature.
         */
        GeomType geometry_type() const noexcept {
            assert(valid());
            return m_geometry_type;
        }

        /**
         * Get the geometry of this feature.
         */
        vtzero::geometry geometry() const noexcept {
            assert(valid());
            return {m_geometry, m_geometry_type};
        }

        /**
         * Returns true if this feature doesn't have any properties.
         *
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_properties_size == 0;
        }

        /**
         * Returns the number of properties in this feature.
         *
         * Complexity: Constant.
         */
        std::size_t size() const noexcept {
            return m_properties_size;
        }

        /// Returns an iterator to the beginning of the properties.
        properties_iterator begin() const noexcept {
            return {m_properties.begin(), m_layer};
        }

        /// Returns an iterator to the end of the properties.
        properties_iterator end() const noexcept {
            return {m_properties.end(), m_layer};
        }

    }; // class feature

    /**
     * Create some kind of mapping from property keys to property values.
     *
     * This can be used to read all properties into a std::map or similar
     * object.
     *
     * @tparam TMap Map type (std::map, std::unordered_map, ...) Must support
     *              the emplace() method.
     * @tparam TKey Key type, usually the key of the map type. The data_view
     *              of the property key is converted to this type before
     *              adding it to the map.
     * @tparam TValue Value type, usally the value of the map type. The
     *                property_value_view is converted to this type before
     *                adding it to the map.
     */
    template <typename TMap, typename TKey = typename TMap::key_type, typename TValue = typename TMap::mapped_type>
    TMap create_properties_map(const vtzero::feature& feature) {
        TMap map;

        for (const auto& p : feature) {
            map.emplace(TKey(p.key()), convert_property_value<TValue>(p.value()));
        }

        return map;
    }

} // namespace vtzero

#endif // VTZERO_FEATURE_HPP
