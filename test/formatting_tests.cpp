
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/detail/geometry.hpp>
#include <vtzero/vector_tile.hpp>

#include <string>

static std::string open_tile(const std::string& path) {
    return load_fixture_tile("BASE_DIR", path);
}

// ---------------------------------------------------------------------------

template <typename E>
void test_layer(const std::string& name, const char* msg) {
    const std::string buffer{open_tile(name + ".mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    REQUIRE_THROWS_AS(*tile.begin(), const E&);
    REQUIRE_THROWS_WITH(*tile.begin(), msg);
}

template <typename E>
void test_feature(const std::string& name, const char* msg) {
    const std::string buffer{open_tile(name + ".mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 1);

    const auto layer = *tile.begin();

    REQUIRE_THROWS_AS(*layer.begin(), const E&);
    REQUIRE_THROWS_WITH(*layer.begin(), msg);
}

// ---------------------------------------------------------------------------

TEST_CASE("unknown fields in tile should be ignored") {
    const std::string buffer{open_tile("unknown_fields_in_tile.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 2);

    auto it = tile.begin();
    {
        const auto layer = *it;
        REQUIRE(layer.name() == "foo");
        REQUIRE(layer.layer_num() == 0);
    }
    ++it;
    {
        const auto layer = *it;
        REQUIRE(layer.name() == "bar");
        REQUIRE(layer.layer_num() == 1);
    }
    ++it;
    REQUIRE(it == tile.end());
}

TEST_CASE("unknown fields in tile should be ignored even if there is no layer") {
    const std::string buffer{open_tile("only_unknown_fields_in_tile.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 0);
    REQUIRE(tile.begin() == tile.end());
}

TEST_CASE("layer field with invalid protobuf type must throw") {
    const std::string buffer{open_tile("layer_with_invalid_protobuf_type.mvt")};
    const vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.count_layers() == 0);

    REQUIRE_THROWS_AS(tile.begin(), const vtzero::format_exception&);
    REQUIRE_THROWS_WITH(tile.begin(), "Layer message has wrong protobuf type");
}

TEST_CASE("layer with version 0 is not understood") {
    test_layer<vtzero::version_exception>("layer_with_version_0", "Layer with unknown version 0 (spec 4.1)");
}

TEST_CASE("layer with version 4 is not understood") {
    test_layer<vtzero::version_exception>("layer_with_version_4", "Layer with unknown version 4 (spec 4.1)");
}

TEST_CASE("layer with two double tables is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_two_double_tables", "More than one double table in layer");
}

TEST_CASE("layer with two float tables is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_two_float_tables", "More than one float table in layer");
}

TEST_CASE("layer with two int tables is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_two_int_tables", "More than one int table in layer");
}

TEST_CASE("layer with version 2 string table is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_string_table", "String table in layer with version <= 2");
}

TEST_CASE("layer with version 2 double table is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_double_table", "Double table in layer with version <= 2");
}

TEST_CASE("layer with version 2 float table is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_float_table", "Float table in layer with version <= 2");
}

TEST_CASE("layer with version 2 int table is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_int_table", "Int table in layer with version <= 2");
}

TEST_CASE("layer with version 2 elevation_scaling is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_elevation_scaling", "Elevation scaling message in layer with version <= 2");
}

TEST_CASE("layer with version 2 attribute_scaling is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_version_2_attribute_scaling", "Attribute scaling message in layer with version <= 2");
}

TEST_CASE("layer with attribute_scaling with wrong offset type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_attribute_scaling_with_wrong_offset_type", "Scaling offset has wrong protobuf type");
}

TEST_CASE("layer with attribute_scaling with wrong multiplier type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_attribute_scaling_with_wrong_multiplier_type", "Scaling multiplier has wrong protobuf type");
}

TEST_CASE("layer with attribute_scaling with wrong base type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_attribute_scaling_with_wrong_base_type", "Scaling base has wrong protobuf type");
}

TEST_CASE("layer with attribute_scaling with unknown field") {
    const std::string buffer{open_tile("layer_with_attribute_scaling_with_unknown_field.mvt")};
    const vtzero::vector_tile tile{buffer};
    const auto layer = tile.get_layer_by_name("testname");
    const auto scaling = layer.attribute_scaling(0);
    REQUIRE(scaling.offset() == 22);
}

TEST_CASE("layer without name is not allowed") {
    test_layer<vtzero::format_exception>("layer_without_name", "Missing name in layer (spec 4.1)");

    const std::string buffer{open_tile("layer_without_name.mvt")};
    const vtzero::vector_tile tile{buffer};
    REQUIRE_THROWS_AS(tile.get_layer_by_name("bar"), const vtzero::format_exception&);
}

TEST_CASE("layer with empty name is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_empty_name", "Missing name in layer (spec 4.1)");

    const std::string buffer{open_tile("layer_without_name.mvt")};
    const vtzero::vector_tile tile{buffer};
    REQUIRE_THROWS_AS(tile.get_layer_by_name("bar"), const vtzero::format_exception&);
}

TEST_CASE("layer with zoom level 100 is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_zoom_level_100", "Zoom level in layer > 32 (spec 4.1)");
}

TEST_CASE("layer with tile_x too large is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_tile_x_too_large", "Tile x value in layer out of range (0 - 7) (spec 4.1)");
}

TEST_CASE("layer with tile_y too large is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_tile_y_too_large", "Tile y value in layer out of range (0 - 7) (spec 4.1)");
}


TEST_CASE("layer with wrong version type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_version_type", "Layer version has wrong protobuf type");
}

TEST_CASE("layer with wrong name type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_name_type", "Layer name has wrong protobuf type");
}

TEST_CASE("layer with wrong features type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_features_type", "Layer features have wrong protobuf type");
}

TEST_CASE("layer with wrong keys type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_keys_type", "Layer keys have wrong protobuf type");
}

TEST_CASE("layer with wrong values type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_values_type", "Layer values have wrong protobuf type");
}

TEST_CASE("layer with wrong extent type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_extent_type", "Layer extent has wrong protobuf type");
}

TEST_CASE("layer with wrong string_values type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_string_values_type", "Layer string_values have wrong protobuf type");
}

TEST_CASE("layer with wrong double_values type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_double_values_type", "Layer double_values have wrong protobuf type");
}

TEST_CASE("layer with wrong float_values type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_float_values_type", "Layer float_values have wrong protobuf type");
}

TEST_CASE("layer with wrong int_values type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_int_values_type", "Layer int_values have wrong protobuf type");
}

TEST_CASE("layer with wrong elevation_scaling type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_elevation_scaling_type", "Layer elevation_scaling has wrong protobuf type");
}

TEST_CASE("layer with wrong attribute_scalings type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_attribute_scalings_type", "Layer attribute_scalings have wrong protobuf type");
}

TEST_CASE("layer with wrong tile_x type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_tile_x_type", "Layer tile_x has wrong protobuf type");
}

TEST_CASE("layer with wrong tile_y type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_tile_y_type", "Layer tile_y has wrong protobuf type");
}

TEST_CASE("layer with wrong tile_zoom type is not allowed") {
    test_layer<vtzero::format_exception>("layer_with_wrong_tile_zoom_type", "Layer tile_zoom has wrong protobuf type");
}

TEST_CASE("unknown field in layer should be ignored") {
    test_layer<vtzero::format_exception>("layer_with_unknown_field", "Missing name in layer (spec 4.1)");
}


TEST_CASE("feature with version 2 attributes field is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_version_2_attributes_field", "Attributes in feature in layer with version <= 2");
}

TEST_CASE("feature with version 2 elevations field is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_version_2_elevations_field", "Elevation in feature in layer with version <= 2");
}

TEST_CASE("feature with version 2 geom_attributes field is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_version_2_geom_attributes_field", "Geometric attribute in feature in layer with version <= 2");
}

TEST_CASE("feature with version 2 string_id field is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_version_2_string_id_field", "String id in feature in layer with version <= 2");
}


TEST_CASE("feature with multiple ids (int, int) is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_ids_int_int", "Feature has more than one id/string_id");
}

TEST_CASE("feature with multiple ids (int, string) is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_ids_int_string", "Feature has more than one id/string_id");
}

TEST_CASE("feature with multiple ids (string, string) is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_ids_string_string", "Feature has more than one id/string_id");
}


TEST_CASE("feature with multiple tags is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_tags", "Feature has more than one tags field");
}

TEST_CASE("feature with multiple attributes is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_attributes", "Feature has more than one attributes field");
}

TEST_CASE("feature with multiple geometric_attributes is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_geometric_attributes", "Feature has more than one geometric attributes field");
}

TEST_CASE("feature with multiple geometries is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_geometries", "Feature has more than one geometry");
}

TEST_CASE("feature with multiple elevations is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_elevations", "Feature has more than one elevations field");
}

TEST_CASE("feature with multiple spline_knots is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_multiple_spline_knots", "Feature has more than one spline knots field");
}

TEST_CASE("feature with spline degree 0 is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_spline_degree_0", "Spline degree in feature must be 2 or 3");
}

TEST_CASE("feature with spline degree 1 is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_spline_degree_1", "Spline degree in feature must be 2 or 3");
}

TEST_CASE("feature with spline degree 4 is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_spline_degree_4", "Spline degree in feature must be 2 or 3");
}


TEST_CASE("feature with tags and attributes is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_tags_and_attributes", "Feature has both tags and attributes field (spec 4.4)");
}

TEST_CASE("feature without geometry is not allowed") {
    test_feature<vtzero::format_exception>("feature_without_geometry", "Missing geometry field in feature (spec 4.3)");
}

TEST_CASE("feature with unknown geometry type is not allowed") {
    test_feature<vtzero::format_exception>("feature_with_unknown_geometry_type", "Unknown geometry type in feature (spec 4.3.5)");
}

TEST_CASE("unknown field in feature should be ignored") {
    test_feature<vtzero::format_exception>("feature_with_unknown_field", "Missing geometry field in feature (spec 4.3)");
}
