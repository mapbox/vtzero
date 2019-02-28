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
#include "types.hpp"
#include "util.hpp"

#include <protozero/pbf_message.hpp>

#include <cstddef>
#include <cstdint>
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

        template <typename T>
        class typed_data_view : public data_view {

        public:

            using iterator_type = T;

            using data_view::data_view;
            using data_view::operator=;

            T it_begin() const noexcept {
                return T{data(), data() + size()};
            }

            T it_end() const noexcept {
                return T{data() + size(), data() + size()};
            }

        }; // class typed_data_view

        enum class id_type {
            no_id      = 0,
            integer_id = 1,
            string_id  = 2
        };

        const layer* m_layer = nullptr;

        uint64_t m_integer_id = 0; // defaults to 0, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L32
        data_view m_string_id{};
        std::size_t m_feature_num = 0;

        typed_data_view<protozero::pbf_reader::const_uint32_iterator> m_geometry{};
        typed_data_view<protozero::pbf_reader::const_sint32_iterator> m_elevations{};
        typed_data_view<protozero::pbf_reader::const_uint64_iterator> m_knots{}; // for splines
        typed_data_view<protozero::pbf_reader::const_uint32_iterator> m_tags{}; // version 2 "tags"
        typed_data_view<protozero::pbf_reader::const_uint64_iterator> m_attributes{}; // version 3 attributes
        typed_data_view<protozero::pbf_reader::const_uint64_iterator> m_geometric_attributes{};

        uint32_t m_spline_degree = 2;

        GeomType m_geometry_type = GeomType::UNKNOWN; // defaults to UNKNOWN, see https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto#L41

        id_type m_id_type = id_type::no_id;

        template <typename THandler>
        void decode_tags_impl(THandler&& handler) const;

        template <typename THandler>
        bool decode_attributes_impl(THandler&& handler) const;

        template <typename THandler>
        bool decode_geometric_attributes_impl(THandler&& handler) const;

        template <int Dimensions, unsigned int MaxGeometricAttributes>
        using geom_decoder_type = detail::geometry_decoder<Dimensions,
              MaxGeometricAttributes,
              decltype(m_geometry)::iterator_type,
              decltype(m_elevations)::iterator_type,
              decltype(m_knots)::iterator_type,
              decltype(m_geometric_attributes)::iterator_type>;

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
        feature(const layer* layer, const data_view data, std::size_t feature_num);

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
         * Return the layer this feature belongs to.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        const layer& get_layer() const noexcept {
            return *m_layer;
        }

        /**
         * Return the index of the layer in the vector tile this feature
         * belongs to.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        std::size_t layer_num() const noexcept;

        /**
         * Return the index of the feature in the vector tile layer.
         *
         * Complexity: Constant.
         *
         * @pre @code valid() @endcode
         */
        std::size_t feature_num() const noexcept {
            return m_feature_num;
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
        uint64_t integer_id() const noexcept {
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
         * Does this feature have a 3d geometry?
         *
         * Complexity: Constant.
         */
        bool has_3d_geometry() const noexcept {
            return !m_elevations.empty();
        }

        /**
         * The number of degrees the spline has.
         *
         * @pre @code geometry_type() == GeomType::SPLINE @endcode
         */
        uint32_t spline_degree() const noexcept {
            return m_spline_degree;
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
         * The knots data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view knots_data() const noexcept {
            return m_knots;
        }

        /**
         * Returns true if this feature has any attributes;
         *
         * Complexity: Constant.
         *
         * Always returns true for invalid features.
         */
        bool has_attributes() const noexcept {
            return !m_tags.empty() ||
                   !m_attributes.empty() ||
                   !m_geometric_attributes.empty();
        }

        /**
         * The vt2 attributes data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view tags_data() const noexcept {
            return m_tags;
        }

        /**
         * The vt3 attributes data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view attributes_data() const noexcept {
            return m_attributes;
        }

        /**
         * The vt3 geometric attributes data of this feature.
         *
         * Complexity: Constant.
         *
         * Always returns an empty data_view for invalid features.
         */
        data_view geometric_attributes_data() const noexcept {
            return m_geometric_attributes;
        }

        /**
         * Decode all normal attributes of this feature.
         *
         * This does not decode geometric attributes. See the functions
         * decode_geometric_attributes() or decode_all_attributes() for that.
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
         * Decode all gemetric attributes of this feature.
         *
         * This does not decode normal attributes. See the functions
         * decode_attributes() or decode_all_attributes() for that.
         *
         * @tparam THandler Handler class.
         * @param handler Handler to call callback functions on.
         * @returns whatever handler.result() returns if that function exists,
         *          void otherwise.
         * @throws out_of_range_exception if there is an error decoding the
         *         data.
         * @pre layer version must be 3.
         */
        template <typename THandler>
        detail::get_result_t<THandler> decode_geometric_attributes(THandler&& handler) const;

        /**
         * Decode all normal and geometric attributes of this feature.
         *
         * If you want to only decode normal or geometric attributes, use
         * decode_attributes() or decode_geometric_attributes(), respectively.
         *
         * @tparam THandler Handler class.
         * @param handler Handler to call callback functions on.
         * @returns whatever handler.result() returns if that function exists,
         *          void otherwise.
         * @throws out_of_range_exception if there is an error decoding the
         *         data.
         * @pre layer version must be 3.
         */
        template <typename THandler>
        detail::get_result_t<THandler> decode_all_attributes(THandler&& handler) const;

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

            constexpr static const int dimensions = std::remove_reference<TGeomHandler>::type::dimensions;
            constexpr static const unsigned int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<dimensions, max> decoder{
                m_geometry.size() / 2,
                m_geometry.it_begin(), m_geometry.it_end(),
                m_elevations.it_begin(), m_elevations.it_end(),
                m_knots.it_begin(), m_knots.it_end(),
                m_geometric_attributes.it_begin(), m_geometric_attributes.it_end()};

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
         * @pre Geometry must be a linestring geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_linestring_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::LINESTRING);

            constexpr static const int dimensions = std::remove_reference<TGeomHandler>::type::dimensions;
            constexpr static const unsigned int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<dimensions, max> decoder{
                m_geometry.size() / 2,
                m_geometry.it_begin(), m_geometry.it_end(),
                m_elevations.it_begin(), m_elevations.it_end(),
                m_knots.it_begin(), m_knots.it_end(),
                m_geometric_attributes.it_begin(), m_geometric_attributes.it_end()};

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
         * @pre Geometry must be a polygon geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_polygon_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::POLYGON);

            constexpr static const int dimensions = std::remove_reference<TGeomHandler>::type::dimensions;
            constexpr static const unsigned int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<dimensions, max> decoder{
                m_geometry.size() / 2,
                m_geometry.it_begin(), m_geometry.it_end(),
                m_elevations.it_begin(), m_elevations.it_end(),
                m_knots.it_begin(), m_knots.it_end(),
                m_geometric_attributes.it_begin(), m_geometric_attributes.it_end()};

            return decoder.decode_polygon(std::forward<TGeomHandler>(geom_handler));
        }

        /**
         * Decode a spline geometry.
         *
         * @tparam TGeomHandler Handler class. See tutorial for details.
         * @param geom_handler An object of TGeomHandler.
         * @returns whatever geom_handler.result() returns if that function
         *          exists, void otherwise
         * @throws geometry_error If there is a problem with the geometry.
         * @pre Geometry must be a spline geometry.
         */
        template <typename TGeomHandler>
        detail::get_result_t<TGeomHandler> decode_spline_geometry(TGeomHandler&& geom_handler) const {
            vtzero_assert(geometry_type() == GeomType::SPLINE);

            constexpr static const int dimensions = std::remove_reference<TGeomHandler>::type::dimensions;
            constexpr static const unsigned int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<dimensions, max> decoder{
                m_geometry.size() / 2,
                m_geometry.it_begin(), m_geometry.it_end(),
                m_elevations.it_begin(), m_elevations.it_end(),
                m_knots.it_begin(), m_knots.it_end(),
                m_geometric_attributes.it_begin(), m_geometric_attributes.it_end()};

            return decoder.decode_spline(std::forward<TGeomHandler>(geom_handler));
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
            constexpr static const int dimensions = std::remove_reference<TGeomHandler>::type::dimensions;
            constexpr static const unsigned int max = std::remove_reference<TGeomHandler>::type::max_geometric_attributes;

            geom_decoder_type<dimensions, max> decoder{
                m_geometry.size() / 2,
                m_geometry.it_begin(), m_geometry.it_end(),
                m_elevations.it_begin(), m_elevations.it_end(),
                m_knots.it_begin(), m_knots.it_end(),
                m_geometric_attributes.it_begin(), m_geometric_attributes.it_end()};

            switch (geometry_type()) {
                case GeomType::POINT:
                    return decoder.decode_point(std::forward<TGeomHandler>(geom_handler));
                case GeomType::LINESTRING:
                    return decoder.decode_linestring(std::forward<TGeomHandler>(geom_handler));
                case GeomType::POLYGON:
                    return decoder.decode_polygon(std::forward<TGeomHandler>(geom_handler));
                case GeomType::SPLINE:
                    return decoder.decode_spline(std::forward<TGeomHandler>(geom_handler));
                default:
                    break;
            }

            throw geometry_exception{"Unknown geometry type in feature (spec 4.3.5)", layer_num(), m_feature_num};
        }

    }; // class feature

} // namespace vtzero

#endif // VTZERO_FEATURE_HPP
