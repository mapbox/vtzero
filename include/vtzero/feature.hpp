#ifndef VTZERO_FEATURE_HPP
#define VTZERO_FEATURE_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file feature.hpp
 *
 * @brief Contains the feature class.
 */

#include "exception.hpp"
#include "geometry.hpp"
#include "property.hpp"
#include "property_value.hpp"
#include "types.hpp"
#include "util.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace vtzero {

    class layer;

    /**
     * A feature according to spec 4.2.
     *
     * Note that a feature will internally contain a pointer to the layer it
     * came from. The layer has to stay valid as long as the feature is used.
     */
    class feature {

        enum class id_type {
            no_id = 0,
            integer_id = 1,
            string_id = 2
        };

        using uint32_it_range = protozero::iterator_range<protozero::pbf_reader::const_uint32_iterator>;
        using uint64_it_range = protozero::iterator_range<protozero::pbf_reader::const_uint64_iterator>;

        const layer* m_layer = nullptr;
        uint64_t m_integer_id = 0; // defaults to 0, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L32
        data_view m_string_id{};
        uint32_it_range m_properties{}; // version 2 "tags"
        uint64_it_range m_attributes{}; // version 3 attributes
        protozero::pbf_reader::const_uint32_iterator m_property_iterator{};
        std::size_t m_num_properties = 0;

        data_view m_geometry{};
        data_view m_elevations{};
        data_view m_geometric_attributes{};
        GeomType m_geometry_type = GeomType::UNKNOWN; // defaults to UNKNOWN, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L41

        id_type m_id_type = id_type::no_id;

        using geom_iterator = protozero::pbf_reader::const_uint32_iterator;
        using elev_iterator = protozero::pbf_reader::const_sint64_iterator;
        using attr_iterator = protozero::pbf_reader::const_uint64_iterator;

        geom_iterator geometry_begin() const noexcept {
            return {m_geometry.data(), m_geometry.data() + m_geometry.size()};
        }

        geom_iterator geometry_end() const noexcept {
            return {m_geometry.data() + m_geometry.size(), m_geometry.data() + m_geometry.size()};
        }

        elev_iterator elevations_begin() const noexcept {
            return {m_elevations.data(), m_elevations.data() + m_elevations.size()};
        }

        elev_iterator elevations_end() const noexcept {
            return {m_elevations.data() + m_elevations.size(), m_elevations.data() + m_elevations.size()};
        }

        attr_iterator geometric_attributes_begin() const noexcept {
            return {m_geometric_attributes.data(), m_geometric_attributes.data() + m_geometric_attributes.size()};
        }

        attr_iterator geometric_attributes_end() const noexcept {
            return {m_geometric_attributes.data() + m_geometric_attributes.size(), m_geometric_attributes.data() + m_geometric_attributes.size()};
        }

        template <int MaxGeometricAttributes>
        using geom_decoder_type = detail::extended_geometry_decoder<3,
              MaxGeometricAttributes,
              geom_iterator,
              protozero::pbf_reader::const_sint64_iterator,
              protozero::pbf_reader::const_uint64_iterator>;

    public:

        /**
         * Construct an invalid feature object.
         */
        feature() = default;

        /**
         * Construct a feature object.
         *
         * @throws format_exception if the layer data is ill-formed.
         */
        feature(const layer* layer, const data_view data);

        /**
         * Is this a valid feature? Valid features are those not created from
         * the default constructor.
         *
         * Complexity: Constant.
         */
        bool valid() const noexcept {
            return m_geometry.data() != nullptr;
        }

        /**
         * Is this a valid feature? Valid features are those not created from
         * the default constructor.
         *
         * Complexity: Constant.
         */
        explicit operator bool() const noexcept {
            return valid();
        }

        /**
         * The integer ID of this feature.
         *
         * Returns 0 if it isn't set.
         *
         * According to the spec IDs should be unique
         * in a layer if they are set (spec 4.2).
         *
         * Complexity: Constant.
         *
         * Always returns 0 for invalid features.
         */
        uint64_t id() const noexcept {
            return m_integer_id;
        }

        /**
         * The string ID of this feature.
         *
         * Returns an empty data_view if this feature doesn't have a string_id.
         *
         * According to the spec IDs should be unique
         * in a layer if they are set (spec 4.2).
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view string_id() const noexcept {
            return m_string_id;
        }

        /**
         * Does this feature have an ID?
         *
         * Complexity: Constant.
         *
         * Always returns false for invalid features.
         */
        bool has_id() const noexcept {
            return m_id_type != id_type::no_id;
        }

        /**
         * Does this feature have an integer ID?
         *
         * Complexity: Constant.
         *
         * Always returns false for invalid features.
         */
        bool has_integer_id() const noexcept {
            return m_id_type == id_type::integer_id;
        }

        /**
         * Does this feature have a string ID?
         *
         * Complexity: Constant.
         *
         * Always returns false for invalid features.
         */
        bool has_string_id() const noexcept {
            return m_id_type == id_type::string_id;
        }

        /**
         * The geometry type of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns GeomType::UNKNOWN for invalid features.
         */
        GeomType geometry_type() const noexcept {
            return m_geometry_type;
        }

        /**
         * The (2D) geometry data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view geometry_data() const noexcept {
            return m_geometry;
        }

        /**
         * The elevations data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view elevations_data() const noexcept {
            return m_elevations;
        }

        /**
         * Does this feature have a 3d geometry?
         *
         * Complexity: Constant.
         */
        bool is_3d() const noexcept {
            return !m_elevations.empty();
        }

        /**
         * Get the geometry of this feature.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        vtzero::geometry geometry() const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            return {m_geometry, m_geometry_type};
        }

        /**
         * Returns true if this feature doesn't have any properties.
         *
         * Complexity: Constant.
         *
         * Always returns true for invalid features.
         */
        bool empty() const noexcept {
            return m_num_properties == 0;
        }

        /**
         * Returns the number of properties in this feature.
         *
         * Complexity: Constant.
         *
         * Always returns 0 for invalid features.
         */
        std::size_t num_properties() const noexcept {
            return m_num_properties;
        }

        /**
         * Get the next property in this feature.
         *
         * Complexity: Constant.
         *
         * @returns The next property or the invalid property if there are no
         *          more properties.
         * @throws format_exception if the feature data is ill-formed.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        property next_property();

        /**
         * Get the indexes into the key/value table for the next property in
         * this feature.
         *
         * Complexity: Constant.
         *
         * @returns The next index_value_pair or an invalid index_value_pair
         *          if there are no more properties.
         * @throws format_exception if the feature data is ill-formed.
         * @throws out_of_range_exception if the key or value index is not
         *         within the range of indexes in the layer key/value table.
         * @throws any protozero exception if the protobuf encoding is invalid.
         * @pre @code valid() @endcode
         */
        index_value_pair next_property_indexes();

        /**
         * Reset the property iterator. The next time next_property() or
         * next_property_indexes() is called, it will begin from the first
         * property again.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        void reset_property() noexcept {
            vtzero_assert_in_noexcept_function(valid());
            m_property_iterator = m_properties.begin();
        }

        /**
         * Call a function for each property of this feature.
         *
         * @tparam TFunc The type of the function. It must take a single
         *         argument of type property&& and return a bool. If the
         *         function returns false, the iteration will be stopped.
         * @param func The function to call.
         * @returns true if the iteration was completed and false otherwise.
         * @pre @code valid() @endcode
         */
        template <typename TFunc>
        bool for_each_property(TFunc&& func) const;

        /**
         * Call a function for each key/value index of this feature.
         *
         * @tparam TFunc The type of the function. It must take a single
         *         argument of type index_value_pair&& and return a bool.
         *         If the function returns false, the iteration will be stopped.
         * @param func The function to call.
         * @returns true if the iteration was completed and false otherwise.
         * @pre @code valid() @endcode
         */
        template <typename TFunc>
        bool for_each_property_indexes(TFunc&& func) const;

        /**
         * Decode all properties in this feature.
         *
         * @tparam THandler Handler class.
         * @param handler Handler to call callback functions on.
         * @returns whatever handler.result() returns if that function exists,
         *          void otherwise.
         * @throws out_of_range_exception if there is an error decoding the
         *         data.
         */
        template <typename THandler>
        detail::get_result_t<THandler> decode_attributes(THandler&& handler) const;

        /**
         * Decode a point geometry.
         *
         * @tparam TGeomHandler Handler class. See tutorial for details.
         * @param geom_handler An object of TGeomHandler.
         * @returns whatever geom_handler.result() returns if that function
         *          exists, void otherwise
         * @throws geometry_error If there is a problem with the geometry.
         * @pre Geometry must be a point geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_point_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::POINT);

            constexpr static const int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<max> decoder{geometry_begin(), geometry_end(),
                elevations_begin(), elevations_end(),
                geometric_attributes_begin(), geometric_attributes_end(),
                m_geometry.size() / 2};

            return decoder.decode_point(std::forward<TGeomHandler>(geom_handler));
        }

        /**
         * Decode a linestring geometry.
         *
         * @tparam TGeomHandler Handler class. See tutorial for details.
         * @param geom_handler An object of TGeomHandler.
         * @returns whatever geom_handler.result() returns if that function
         *          exists, void otherwise
         * @throws geometry_error If there is a problem with the geometry.
         * @pre Geometry must be a point geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_linestring_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::LINESTRING);

            constexpr static const int max = std::decay<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<max> decoder{geometry_begin(), geometry_end(),
                elevations_begin(), elevations_end(),
                geometric_attributes_begin(), geometric_attributes_end(),
                m_geometry.size() / 2};

            return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
        }

        /**
         * Decode a polygon geometry.
         *
         * @tparam TGeomHandler Handler class. See tutorial for details.
         * @param geom_handler An object of TGeomHandler.
         * @returns whatever geom_handler.result() returns if that function
         *          exists, void otherwise
         * @throws geometry_error If there is a problem with the geometry.
         * @pre Geometry must be a point geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_polygon_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::POLYGON);

            constexpr static const int max = std::decay<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<max> decoder{geometry_begin(), geometry_end(),
                elevations_begin(), elevations_end(),
                geometric_attributes_begin(), geometric_attributes_end(),
                m_geometry.size() / 2};

            return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
        }

        /**
         * Decode a geometry.
         *
         * @tparam TGeomHandler Handler class. See tutorial for details.
         * @param geom_handler An object of TGeomHandler.
         * @returns whatever geom_handler.result() returns if that function
         *          exists, void otherwise
         * @throws geometry_error If the geometry has type UNKNOWN of if there
         *                        is a problem with the geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_geometry(TGeomHandler&& geom_handler) const {
            constexpr static const int max = std::decay<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<max> decoder{geometry_begin(), geometry_end(),
                elevations_begin(), elevations_end(),
                geometric_attributes_begin(), geometric_attributes_end(),
                m_geometry.size() / 2};

            switch (geometry_type()) {
                case GeomType::POINT:
                    return decoder.decode_point(std::forward<TGeomHandler>(geom_handler));
                case GeomType::LINESTRING:
                    return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
                case GeomType::POLYGON:
                    return decoder.decode_polygon(std::forward<TGeomHandler>(geom_handler));
                default:
                    break;
            }

            throw geometry_exception{"unknown geometry type"};
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
     *                property_value is converted to this type before
     *                adding it to the map.
     * @tparam TMapping A struct derived from property_value_mapping with the
     *         mapping for vtzero property value types to TValue-constructing
     *         types. (See convert_property_value() for details.)
     * @param feature The feature to get the properties from.
     * @returns An object of type TMap with all the properties.
     * @pre @code feature.valid() @endcode
     */
    template <typename TMap,
              typename TKey = typename TMap::key_type,
              typename TValue = typename TMap::mapped_type,
              typename TMapping = property_value_mapping>
    TMap create_properties_map(const vtzero::feature& feature) {
        TMap map;

        feature.for_each_property([&map](const property& p) {
            map.emplace(TKey(p.key()), convert_property_value<TValue, TMapping>(p.value()));
            return true;
        });

        return map;
    }

} // namespace vtzero

#endif // VTZERO_FEATURE_HPP
