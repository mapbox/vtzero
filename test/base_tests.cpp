
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/detail/geometry.hpp>
#include <vtzero/vector_tile.hpp>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static std::string open_tile(const std::string& path) {
    return load_fixture_tile("BASE_DIR", path);
}

template <int Dimensions>
class geom_handler {

    std::string m_geom;
    const vtzero::layer& m_layer;
    vtzero::index_value m_scaling;

    void add_point(const vtzero::point_2d point) {
        m_geom += "(";
        m_geom += std::to_string(point.x);
        m_geom += ",";
        m_geom += std::to_string(point.y);
        m_geom += ")";
    }

    void add_point(const vtzero::point_3d point) {
        m_geom += "(";
        m_geom += std::to_string(point.x);
        m_geom += ",";
        m_geom += std::to_string(point.y);
        m_geom += ",";
        m_geom += std::to_string(point.z);
        m_geom += ")";
    }

public:

    constexpr static const int dimensions = Dimensions;
    constexpr static const unsigned int max_geometric_attributes = 0;

    geom_handler(const vtzero::layer& layer) :
        m_layer(layer) {
    }

    void points_begin(const uint32_t /*count*/) {
        m_geom += "P(";
    }

    void points_point(const vtzero::point_2d point) {
        add_point(point);
    }

    void points_point(const vtzero::point_3d point) {
        add_point(point);
    }

    void points_end() {
        m_geom += ")";
    }

    void linestring_begin(const uint32_t /*count*/) {
        m_geom += "L(";
    }

    void linestring_point(const vtzero::point_2d point) {
        add_point(point);
    }

    void linestring_point(const vtzero::point_3d point) {
        add_point(point);
    }

    void linestring_end() {
        m_geom += ")";
    }

    void ring_begin(const uint32_t /*count*/) {
        m_geom += "R(";
    }

    void ring_point(const vtzero::point_2d point) {
        add_point(point);
    }

    void ring_point(const vtzero::point_3d point) {
        add_point(point);
    }

    void ring_end(const vtzero::ring_type type) {
        if (type == vtzero::ring_type::inner) {
            m_geom += "[inner])";
        } else {
            m_geom += "[outer])";
        }
    }

    void controlpoints_begin(const uint32_t /*count*/) {
        m_geom += "C(";
    }

    void controlpoints_point(const vtzero::point_2d point) {
        add_point(point);
    }

    void controlpoints_point(const vtzero::point_3d point) {
        add_point(point);
    }

    void controlpoints_end() {
        m_geom += ")";
    }

    void knots_begin(const uint32_t /*count*/, const vtzero::index_value scaling) {
        m_scaling = scaling;
        m_geom += "K([scaling=";
        m_geom += std::to_string(scaling.value());
        m_geom += "]";
    }

    void knots_value(const int64_t value) {
        m_geom += "(";
        m_geom += std::to_string(m_layer.attribute_scaling(m_scaling).decode(value));
        m_geom += ")";
    }

    void knots_end() {
        m_geom += ")";
    }

    std::string result() const {
        return m_geom;
    }

}; // class geom_handler

// ---------------------------------------------------------------------------

TEST_CASE("valid/all_attribute_types_v3.mvt") {
    const std::string buffer{open_tile("valid/all_attribute_types_v3.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "example");
    REQUIRE(layer.num_features() == 1);
    REQUIRE(layer.num_attribute_scalings() == 1);
    REQUIRE(layer.attribute_scaling(0).offset() == 0);
    REQUIRE(layer.attribute_scaling(0).multiplier() == Approx(7.45058e-09));
    REQUIRE(layer.attribute_scaling(0).base() == Approx(0.0));

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
    REQUIRE_FALSE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 1);
}

// ---------------------------------------------------------------------------

TEST_CASE("valid/single_layer_v2_linestring.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v2_linestring.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.name() == "lines");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);
    REQUIRE_FALSE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 6);

    geom_handler<2> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "L((10,10)(10,20)(20,20))L((11,11)(12,13))");
}

TEST_CASE("valid/single_layer_v2_points.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v2_points.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.name() == "points");
    REQUIRE(layer.num_features() == 4);

    auto it = layer.begin();

    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE_FALSE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 2);

        geom_handler<2> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE_FALSE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 3);

        geom_handler<2> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE_FALSE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 4);

        geom_handler<2> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE_FALSE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 5);

        geom_handler<2> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20))");
    }

    REQUIRE(++it == layer.end());
}

TEST_CASE("valid/single_layer_v2_polygon.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v2_polygon.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.name() == "polygons");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::POLYGON);
    REQUIRE_FALSE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 7);

    geom_handler<2> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "R((0,0)(10,0)(10,10)(0,10)(0,0)[outer])R((3,3)(3,5)(5,5)(3,3)[inner])");
}

TEST_CASE("valid/single_layer_v3_linestring_3d.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v3_linestring_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "lines_3d");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);
    REQUIRE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 14);

    geom_handler<3> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "L((10,10,10)(10,20,20)(20,20,30))L((11,11,10)(12,13,20))");
}

TEST_CASE("valid/single_layer_v3_points_3d.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v3_points_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "points_3d");
    REQUIRE(layer.num_features() == 4);

    auto it = layer.begin();
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 10);

        geom_handler<3> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20,10))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 11);

        geom_handler<3> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20,20))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 12);

        geom_handler<3> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20,30))");
    }

    ++it;
    {
        const auto feature = *it;
        REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);
        REQUIRE(feature.has_3d_geometry());
        REQUIRE(feature.integer_id() == 13);

        geom_handler<3> handler{layer};
        REQUIRE(feature.decode_geometry(handler) == "P((20,20,40))");
    }

    REQUIRE(++it == layer.end());
}

TEST_CASE("valid/single_layer_v3_spline_3d.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v3_spline_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "splines_3d");
    REQUIRE(layer.num_features() == 1);
    REQUIRE(layer.num_attribute_scalings() == 1);
    REQUIRE(layer.attribute_scaling(0).offset() == 0);
    REQUIRE(layer.attribute_scaling(0).multiplier() == Approx(7.45058e-09));
    REQUIRE(layer.attribute_scaling(0).base() == Approx(0.0));

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::SPLINE);
    REQUIRE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 16);

    geom_handler<3> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "C((8,10,10)(9,11,11)(11,9,12)(12,10,13))K([scaling=0](0.000000)(2.000000)(3.000000)(4.000000)(5.875000)(6.000000)(7.000000)(8.000000))");
}

TEST_CASE("valid/single_layer_v3_spline.mvt") {
    const std::string buffer{open_tile("valid/single_layer_v3_spline.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "splines");
    REQUIRE(layer.num_features() == 1);
    REQUIRE(layer.num_attribute_scalings() == 1);
    REQUIRE(layer.attribute_scaling(0).offset() == 0);
    REQUIRE(layer.attribute_scaling(0).multiplier() == Approx(7.45058e-09));
    REQUIRE(layer.attribute_scaling(0).base() == Approx(0.0));

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::SPLINE);
    REQUIRE_FALSE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 8);

    geom_handler<2> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "C((8,10)(9,11)(11,9)(12,10))K([scaling=0](0.000000)(2.000000)(3.000000)(4.000000)(5.875000)(6.000000)(7.000000)(8.000000))");
}

// ---------------------------------------------------------------------------

TEST_CASE("invalid/single_layer_v3_polygon_3d.mvt") {
    const std::string buffer{open_tile("invalid/single_layer_v3_polygon_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "polygons_3d");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::POLYGON);
    REQUIRE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 15);

    geom_handler<3> handler{layer};
    REQUIRE(feature.decode_geometry(handler) == "R((0,0,10)(10,0,20)(10,10,30)(0,10,20)(0,0,10)[outer])R((3,3,20)(3,5,40)(5,5,30)(3,3,20)[inner])");
}

