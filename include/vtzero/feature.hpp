#ifndef VTZERO_FEATURE_HPP
#define VTZERO_FEATURE_HPP

#include "exception.hpp"
#include "types.hpp"
#include "value_view.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

namespace vtzero {

    /**
     * A vector tile property.
     */
    class property_view {

        data_view m_key;
        value_view m_value;

    public:

        property_view() = default;

        property_view(const data_view& key, const value_view& value) noexcept :
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

    }; // class property_view

    class layer;

    class properties_iterator {

        protozero::pbf_reader::const_uint32_iterator m_it;
        protozero::pbf_reader::const_uint32_iterator m_end;
        const layer* m_layer;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = property_view;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type&;

        properties_iterator(const protozero::pbf_reader::const_uint32_iterator& begin,
                      const protozero::pbf_reader::const_uint32_iterator& end,
                      const layer* layer) :
            m_it(begin),
            m_end(end),
            m_layer(layer) {
            assert(layer);
        }

        property_view operator*() const;

        properties_iterator& operator++() {
            assert(m_it != m_end);
            if (std::next(m_it) == m_end) {
                throw format_exception{"unpaired property key/value indexes (spec 4.4)"};
            }
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
                   m_end == other.m_end &&
                   m_layer == other.m_layer;
        }

        bool operator!=(const properties_iterator& other) const noexcept {
            return !(*this == other);
        }

        /**
         * Returns true if there are no properties.
         *
         * Complexity: Constant.
         */
        bool empty() const noexcept {
            return m_it == m_end;
        }

        /**
         * Return the number of properties.
         *
         * Complexity: Linear.
         */
        std::size_t size() const {
            if (m_it.size() % 2 != 0) {
                throw format_exception{"unpaired property key/value indexes (spec 4.4)"};
            }
            return m_it.size() / 2;
        }

    }; // properties_iterator

    /**
     * A feature according to spec 4.2.
     */
    class feature {

        using uint32_it_range = protozero::iterator_range<protozero::pbf_reader::const_uint32_iterator>;

        uint64_t m_id;
        uint32_it_range m_properties;
        data_view m_geometry;
        GeomType m_type;
        bool m_has_id;

    public:

        feature() :
            m_id(0),
            m_properties(),
            m_geometry(),
            m_type(),
            m_has_id(false) {
        }

        feature(const data_view& data) :
            m_id(0), // defaults to 0, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L32
            m_properties(),
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
                        if (m_properties.begin() != protozero::pbf_reader::const_uint32_iterator{}) {
                            throw format_exception{"Feature has more than one tags field"}; // XXX or do we need to handle this?
                        }
                        m_properties = reader.get_packed_uint32();
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
                        if (m_geometry.size() != 0) {
                            throw format_exception{"Feature has more than one geometry field"}; // XXX or do we need to handle this?
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

        protozero::iterator_range<properties_iterator> properties(const layer& layer) const noexcept {
            return {{m_properties.begin(), m_properties.end(), &layer},
                    {m_properties.end(), m_properties.end(), &layer}};
        }

    }; // class feature


    template <typename TMap, typename TKey = typename TMap::key_type, typename TValue = typename TMap::mapped_type>
    TMap create_properties_map(const vtzero::feature& feature, const vtzero::layer& layer) {
        TMap map;

        for (const auto& p : feature.properties(layer)) {
            map.emplace(TKey{p.key()}, convert_value<TValue>(p.value()));
        }

        return map;
    }

} // namespace vtzero

#endif // VTZERO_FEATURE_HPP
