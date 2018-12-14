
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/vector_tile.hpp>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static std::string open_tile(const std::string& path) {
    const auto fixtures_dir = std::getenv("BASE_DIR");
    if (fixtures_dir == nullptr) {
        std::cerr << "Set BASE_DIR environment variable to the directory where the mvt fixtures are!\n";
        std::exit(2);
    }

    std::ifstream stream{std::string{fixtures_dir} + "/" + path,
                         std::ios_base::in|std::ios_base::binary};
    if (!stream.is_open()) {
        throw std::runtime_error{"could not open: '" + path + "'"};
    }

    const std::string message{std::istreambuf_iterator<char>(stream.rdbuf()),
                              std::istreambuf_iterator<char>()};

    stream.close();
    return message;
}

// ---------------------------------------------------------------------------

TEST_CASE("valid/all_attribute_types_v3.mvt") {
    std::string buffer{open_tile("valid/all_attribute_types_v3.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "example");
    REQUIRE(layer.num_features() == 1);
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
    std::string buffer{open_tile("valid/single_layer_v2_linestring.mvt")};
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
}

TEST_CASE("valid/single_layer_v2_points.mvt") {
    std::string buffer{open_tile("valid/single_layer_v2_points.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.name() == "points");
    REQUIRE(layer.num_features() == 4);
}

TEST_CASE("valid/single_layer_v2_polygon.mvt") {
    std::string buffer{open_tile("valid/single_layer_v2_polygon.mvt")};
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
}

TEST_CASE("valid/single_layer_v3_linestring_3d.mvt") {
    std::string buffer{open_tile("valid/single_layer_v3_linestring_3d.mvt")};
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
}

TEST_CASE("valid/single_layer_v3_points_3d.mvt") {
    std::string buffer{open_tile("valid/single_layer_v3_points_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "points_3d");
    REQUIRE(layer.num_features() == 4);
}

TEST_CASE("valid/single_layer_v3_spline_3d.mvt") {
    std::string buffer{open_tile("valid/single_layer_v3_spline_3d.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "splines_3d");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::SPLINE);
    REQUIRE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 16);
}

TEST_CASE("valid/single_layer_v3_spline.mvt") {
    std::string buffer{open_tile("valid/single_layer_v3_spline.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.name() == "splines");
    REQUIRE(layer.num_features() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.geometry_type() == vtzero::GeomType::SPLINE);
    REQUIRE_FALSE(feature.has_3d_geometry());
    REQUIRE(feature.integer_id() == 8);
}

// ---------------------------------------------------------------------------

TEST_CASE("invalid/single_layer_v3_polygon_3d.mvt") {
    std::string buffer{open_tile("invalid/single_layer_v3_polygon_3d.mvt")};
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
}

