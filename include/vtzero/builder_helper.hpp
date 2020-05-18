#ifndef VTZERO_BUILDER_HELPER_HPP
#define VTZERO_BUILDER_HELPER_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file builder_helper.hpp
 *
 * @brief Contains functions to build geometries from containers.
 */

#include "builder.hpp"
#include "exception.hpp"

#include <cstdint>

namespace vtzero {

    /**
     * Copy a feature from an existing layer to a new layer. The feature
     * will be copied completely over to the new layer including its id,
     * geometry and all its attributes.
     */
    template <typename TLayerBuilder>
    void copy_feature(const feature& feature, TLayerBuilder& layer_builder) {
        feature_builder<3> feature_builder{layer_builder};
        feature_builder.copy_id(feature);
        feature_builder.copy_geometry(feature);
        feature_builder.copy_attributes(feature);
        feature_builder.commit();
    }

    namespace detail {

        /// Helper function to check size isn't too large
        template <typename T>
        static uint32_t check_num_points(T size) {
            if (size >= (1UL << 29U)) {
                throw geometry_exception{"Maximum of 2^29 - 1 points allowed in geometry"};
            }
            return static_cast<uint32_t>(size);
        }

    } // namespace detail

    /**
     * Add the points from the specified container as multipoint geometry
     * to this feature.
     *
     * @tparam TContainer The container type. Must support the size()
     *         method, be iterable using a range for loop, and contain
     *         objects of type vtzero::point or something convertible to
     *         it.
     * @param container The container to read the points from.
     * @param builder The feature builder to add to.
     *
     * @throws geometry_exception If there are more than 2^32-1 members in
     *         the container.
     *
     * @pre Feature builder must be in stage "id" or "has_id" to call this function.
     */
    template <typename TContainer, int Dimensions>
    void add_points_from_container(const TContainer& container, point_feature_builder<Dimensions>& builder) {
        builder.add_points(detail::check_num_points(container.size()));
        for (const auto& element : container) {
            builder.set_point(element);
        }
    }

    /**
     * Add the points from the specified container as a linestring geometry
     * to this feature.
     *
     * @tparam TContainer The container type. Must support the size()
     *         method, be iterable using a range for loop, and contain
     *         objects of type vtzero::point or something convertible to
     *         it.
     * @param container The container to read the points from.
     * @param builder The feature builder to add to.
     *
     * @throws geometry_exception If there are more than 2^32-1 members in
     *         the container or if two consecutive points in the container
     *         are identical.
     *
     * @pre Feature builder must be in stage "id", "has_id", or "geometry" to
     *      call this function.
     */
    template <typename TContainer, int Dimensions>
    void add_linestring_from_container(const TContainer& container, linestring_feature_builder<Dimensions>& builder) {
        builder.add_linestring(detail::check_num_points(container.size()));
        for (const auto& element : container) {
            builder.set_point(element);
        }
    }

    /**
     * Add the points from the specified container as a ring to this
     * feature.
     *
     * @tparam TContainer The container type. Must support the size()
     *         method, be iterable using a range for loop, and contain
     *         objects of type vtzero::point or something convertible to
     *         it.
     * @param container The container to read the points from.
     * @param builder The feature builder to add to.
     *
     * @throws geometry_exception If there are more than 2^32-1 members in
     *         the container or if two consecutive points in the container
     *         are identical or if the last point is not the same as the
     *         first point.
     *
     * @pre Feature builder must be in stage "id", "has_id", or "geometry" to
     *      call this function.
     */
    template <typename TContainer, int Dimensions>
    void add_ring_from_container(const TContainer& container, polygon_feature_builder<Dimensions>& builder) {
        builder.add_ring(detail::check_num_points(container.size()));
        for (const auto& element : container) {
            builder.set_point(element);
        }
    }

} // namespace vtzero

#endif // VTZERO_BUILDER_HELPER_HPP
