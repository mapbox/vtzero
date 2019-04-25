#ifndef TEST_GEOMETRY_HANDLER_HPP
#define TEST_GEOMETRY_HANDLER_HPP

#include <vtzero/detail/geometry.hpp>

#include <vector>

template <int Dimensions>
struct point_handler {

    constexpr static const int dimensions = Dimensions;
    constexpr static const unsigned int max_geometric_attributes = 0;

    using result_type = std::vector<vtzero::point<Dimensions>>;
    result_type data{};

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point<Dimensions> point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

}; // struct point_handler

template <int Dimensions>
struct linestring_handler {

    constexpr static const int dimensions = Dimensions;
    constexpr static const unsigned int max_geometric_attributes = 0;

    using result_type = std::vector<std::vector<vtzero::point<Dimensions>>>;
    result_type data{};

    void linestring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void linestring_point(const vtzero::point<Dimensions> point) {
        data.back().push_back(point);
    }

    void linestring_end() const noexcept {
    }

}; // struct linestring_handler

template <int Dimensions>
struct polygon_handler {

    constexpr static const int dimensions = Dimensions;
    constexpr static const unsigned int max_geometric_attributes = 0;

    using result_type = std::vector<std::vector<vtzero::point<Dimensions>>>;

    result_type data{};

    void ring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void ring_point(const vtzero::point<Dimensions> point) {
        data.back().push_back(point);
    }

    void ring_end(vtzero::ring_type /*dummy*/) const noexcept {
    }

}; // struct polygon_handler

template <int Dimensions>
struct spline_handler {

    constexpr static const int dimensions = Dimensions;
    constexpr static const unsigned int max_geometric_attributes = 0;

    std::vector<std::vector<vtzero::point<Dimensions>>> control_points{};
    std::vector<int64_t> knots{};

    void controlpoints_begin(uint32_t count) {
        control_points.emplace_back();
        control_points.back().reserve(count);
    }

    void controlpoints_point(const vtzero::point<Dimensions> point) {
        control_points.back().push_back(point);
    }

    void controlpoints_end() const noexcept {
    }

    void knots_begin(const uint32_t count, const vtzero::index_value /*scaling*/) {
        knots.reserve(count);
    }

    void knots_value(const int64_t value) {
        knots.push_back(value);
    }

    void knots_end() const noexcept {
    }

}; // struct spline_handler

// ---------------------------------------------------------------------------

struct geom_handler {

    constexpr static const int dimensions = 2;
    constexpr static const unsigned int max_geometric_attributes = 0;

    std::vector<vtzero::point_2d> point_data{};
    std::vector<std::vector<vtzero::point_2d>> line_data{};
    std::vector<vtzero::point_2d> control_points{};
    std::vector<int64_t> knots{};

    void points_begin(uint32_t count) {
        point_data.reserve(count);
    }

    void points_point(const vtzero::point_2d point) {
        point_data.push_back(point);
    }

    void points_end() const noexcept {
    }

    void linestring_begin(uint32_t count) {
        line_data.emplace_back();
        line_data.back().reserve(count);
    }

    void linestring_point(const vtzero::point_2d point) {
        line_data.back().push_back(point);
    }

    void linestring_end() const noexcept {
    }

    void ring_begin(uint32_t count) {
        line_data.emplace_back();
        line_data.back().reserve(count);
    }

    void ring_point(const vtzero::point_2d point) {
        line_data.back().push_back(point);
    }

    void ring_end(vtzero::ring_type /*dummy*/) const noexcept {
    }

    void controlpoints_begin(uint32_t count) {
        control_points.reserve(count);
    }

    void controlpoints_point(const vtzero::point_2d p) {
        control_points.push_back(p);
    }

    void controlpoints_end() const noexcept {
    }

    void knots_begin(const uint32_t count, const vtzero::index_value /*scaling*/) {
        knots.reserve(count);
    }

    void knots_value(const int64_t value) {
        knots.push_back(value);
    }

    void knots_end() const noexcept {
    }

}; // struct geom_handler

#endif // TEST_GEOMETRY_HANDLER_HPP
