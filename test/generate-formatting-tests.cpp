/*****************************************************************************

  generate-tests.cpp

  This is used to generate the test cases in test/data/formatting-tests.

*****************************************************************************/

#include <vtzero/types.hpp>

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_writer.hpp>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

static std::string dir;

static void write_layer_file(const std::string& name, const std::string& tile_data) {
    std::string filename{dir + "/" + name + ".mvt"};
    std::ofstream file{filename, std::ios::out | std::ios::binary | std::ios::trunc};

    file.write(tile_data.data(), static_cast<std::streamsize>(tile_data.size()));
    file.close();

    if (!file) {
        std::cerr << "Error writing to '" << filename << "'.\n";
        std::exit(1);
    }
}

static void write_layer(const std::string& name, const std::string& data) {
    std::string tile_data;
    protozero::pbf_builder<vtzero::detail::pbf_tile> tile_builder{tile_data};
    tile_builder.add_bytes(vtzero::detail::pbf_tile::layers, data);
    write_layer_file(name, tile_data);
}

static std::string wrap_in_layer(uint32_t version, const std::string& feature_data) {
    std::string data;

    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};
    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, version);
    layer_builder.add_string(vtzero::detail::pbf_layer::name, "foo");
    layer_builder.add_message(vtzero::detail::pbf_layer::features, feature_data);

    return data;
}

/****************************************************************************/

static void unknown_fields_in_tile() {
    std::string layer_data_1;
    {
        protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{layer_data_1};
        layer_builder.add_string(vtzero::detail::pbf_layer::name, "foo");
    }
    std::string layer_data_2;
    {
        protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{layer_data_2};
        layer_builder.add_string(vtzero::detail::pbf_layer::name, "bar");
    }

    std::string tile_data;
    protozero::pbf_writer tile_builder{tile_data};
    tile_builder.add_uint32(345, 17);
    tile_builder.add_string(346, "x");
    tile_builder.add_bytes(static_cast<protozero::pbf_tag_type>(vtzero::detail::pbf_tile::layers), layer_data_1);
    tile_builder.add_uint32(345, 31);
    tile_builder.add_bytes(static_cast<protozero::pbf_tag_type>(vtzero::detail::pbf_tile::layers), layer_data_2);
    tile_builder.add_uint32(345, 13);

    write_layer_file("unknown_fields_in_tile", tile_data);
}

static void only_unknown_fields_in_tile() {
    std::string tile_data;
    protozero::pbf_writer tile_builder{tile_data};
    tile_builder.add_uint32(345, 17);
    tile_builder.add_string(346, "x");

    write_layer_file("only_unknown_fields_in_tile", tile_data);
}

static void layer_with_invalid_protobuf_type() {
    std::string tile_data;
    protozero::pbf_builder<vtzero::detail::pbf_tile> tile_builder{tile_data};
    tile_builder.add_uint32(vtzero::detail::pbf_tile::layers, 123); // wrong protobuf type

    write_layer_file("layer_with_invalid_protobuf_type", tile_data);
}

/****************************************************************************/

static void layer_with_version(const uint32_t version) {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, version);

    write_layer("layer_with_version_" + std::to_string(version), data);
}

/****************************************************************************/

static void layer_with_two_double_tables() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    std::array<double, 2> x{{1.1, 2.2}};
    layer_builder.add_packed_double(vtzero::detail::pbf_layer::double_values, x.cbegin(), x.cend());
    layer_builder.add_packed_double(vtzero::detail::pbf_layer::double_values, x.cbegin(), x.cend());

    write_layer("layer_with_two_double_tables", data);
}

static void layer_with_two_float_tables() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    std::array<float, 2> x{{1.1f, 2.2f}};
    layer_builder.add_packed_float(vtzero::detail::pbf_layer::float_values, x.cbegin(), x.cend());
    layer_builder.add_packed_float(vtzero::detail::pbf_layer::float_values, x.cbegin(), x.cend());

    write_layer("layer_with_two_float_tables", data);
}

static void layer_with_two_int_tables() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    std::array<int, 2> x{{1, 2}};
    layer_builder.add_packed_fixed64(vtzero::detail::pbf_layer::int_values, x.cbegin(), x.cend());
    layer_builder.add_packed_fixed64(vtzero::detail::pbf_layer::int_values, x.cbegin(), x.cend());

    write_layer("layer_with_two_int_tables", data);
}

/****************************************************************************/

static void layer_with_version_2_string_table() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    layer_builder.add_string(vtzero::detail::pbf_layer::string_values, "foo");

    write_layer("layer_with_version_2_string_table", data);
}

static void layer_with_version_2_double_table() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    std::array<double, 2> x{{1.1, 2.2}};
    layer_builder.add_packed_double(vtzero::detail::pbf_layer::double_values, x.cbegin(), x.cend());

    write_layer("layer_with_version_2_double_table", data);
}

static void layer_with_version_2_float_table() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    std::array<float, 2> x{{1.1f, 2.2f}};
    layer_builder.add_packed_float(vtzero::detail::pbf_layer::float_values, x.cbegin(), x.cend());

    write_layer("layer_with_version_2_float_table", data);
}

static void layer_with_version_2_int_table() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    std::array<int, 2> x{{1, 2}};
    layer_builder.add_packed_fixed64(vtzero::detail::pbf_layer::int_values, x.cbegin(), x.cend());

    write_layer("layer_with_version_2_int_table", data);
}

static void layer_with_version_2_elevation_scaling() {
    std::string scaling_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_scaling> scaling_builder{scaling_data};
        scaling_builder.add_double(vtzero::detail::pbf_scaling::multiplier, 3.141);
    }

    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    layer_builder.add_message(vtzero::detail::pbf_layer::elevation_scaling, scaling_data);

    write_layer("layer_with_version_2_elevation_scaling", data);
}

static void layer_with_version_2_attribute_scaling() {
    std::string scaling_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_scaling> scaling_builder{scaling_data};
        scaling_builder.add_double(vtzero::detail::pbf_scaling::multiplier, 3.141);
    }

    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};
    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, 2);
    layer_builder.add_message(vtzero::detail::pbf_layer::attribute_scalings, scaling_data);

    write_layer("layer_with_version_2_attribute_scaling", data);
}

/****************************************************************************/

static std::string create_scaling(const std::string& scaling_data, uint32_t version = 3) {
    std::string data;

    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};
    layer_builder.add_uint32(vtzero::detail::pbf_layer::version, version);
    layer_builder.add_string(vtzero::detail::pbf_layer::name, "testname");
    layer_builder.add_message(vtzero::detail::pbf_layer::attribute_scalings, scaling_data);

    return data;
}

static void layer_with_attribute_scaling_with_wrong_offset_type() {
    std::string scaling_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_scaling> scaling_builder{scaling_data};
        scaling_builder.add_double(vtzero::detail::pbf_scaling::offset, 3.141);
    }

    write_layer("layer_with_attribute_scaling_with_wrong_offset_type", create_scaling(scaling_data));
}

static void layer_with_attribute_scaling_with_wrong_multiplier_type() {
    std::string scaling_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_scaling> scaling_builder{scaling_data};
        scaling_builder.add_uint32(vtzero::detail::pbf_scaling::multiplier, 17);
    }

    write_layer("layer_with_attribute_scaling_with_wrong_multiplier_type", create_scaling(scaling_data));
}

static void layer_with_attribute_scaling_with_wrong_base_type() {
    std::string scaling_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_scaling> scaling_builder{scaling_data};
        scaling_builder.add_uint32(vtzero::detail::pbf_scaling::base, 17);
    }

    write_layer("layer_with_attribute_scaling_with_wrong_base_type", create_scaling(scaling_data));
}

static void layer_with_attribute_scaling_with_unknown_field() {
    std::string scaling_data;
    {
        protozero::pbf_writer scaling_builder{scaling_data};
        scaling_builder.add_sint64(protozero::pbf_tag_type(vtzero::detail::pbf_scaling::offset), 22);
        scaling_builder.add_uint32(123, 33);
    }

    write_layer("layer_with_attribute_scaling_with_unknown_field", create_scaling(scaling_data));
}

/****************************************************************************/

static void layer_without_name() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_zoom, 4);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_x, 1);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_y, 1);

    write_layer("layer_without_name", data);
}

static void layer_with_empty_name() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_string(vtzero::detail::pbf_layer::name, "");
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_zoom, 4);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_x, 1);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_y, 1);

    write_layer("layer_with_empty_name", data);
}

/****************************************************************************/

static void layer_with_zoom_level_100() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_zoom, 100);

    write_layer("layer_with_zoom_level_100", data);
}

static void layer_with_tile_x_too_large() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_string(vtzero::detail::pbf_layer::name, "test_layer");
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_zoom, 3);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_x, 100);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_y, 1);

    write_layer("layer_with_tile_x_too_large", data);
}

static void layer_with_tile_y_too_large() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_string(vtzero::detail::pbf_layer::name, "test_layer");
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_zoom, 3);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_x, 1);
    layer_builder.add_uint32(vtzero::detail::pbf_layer::tile_y, 100);

    write_layer("layer_with_tile_y_too_large", data);
}

/****************************************************************************/

static void layer_with_wrong_version_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_fixed32(vtzero::detail::pbf_layer::version, 42);

    write_layer("layer_with_wrong_version_type", data);
}

static void layer_with_wrong_name_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::name, 42);

    write_layer("layer_with_wrong_name_type", data);
}

static void layer_with_wrong_features_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::features, 42);

    write_layer("layer_with_wrong_features_type", data);
}

static void layer_with_wrong_keys_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::keys, 42);

    write_layer("layer_with_wrong_keys_type", data);
}

static void layer_with_wrong_values_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::values, 42);

    write_layer("layer_with_wrong_values_type", data);
}

static void layer_with_wrong_extent_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_fixed32(vtzero::detail::pbf_layer::extent, 42);

    write_layer("layer_with_wrong_extent_type", data);
}

static void layer_with_wrong_string_values_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::string_values, 42);

    write_layer("layer_with_wrong_string_values_type", data);
}

static void layer_with_wrong_double_values_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::double_values, 42);

    write_layer("layer_with_wrong_double_values_type", data);
}

static void layer_with_wrong_float_values_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::float_values, 42);

    write_layer("layer_with_wrong_float_values_type", data);
}

static void layer_with_wrong_int_values_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::int_values, 42);

    write_layer("layer_with_wrong_int_values_type", data);
}

static void layer_with_wrong_elevation_scaling_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::elevation_scaling, 42);

    write_layer("layer_with_wrong_elevation_scaling_type", data);
}

static void layer_with_wrong_attribute_scalings_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_uint32(vtzero::detail::pbf_layer::attribute_scalings, 42);

    write_layer("layer_with_wrong_attribute_scalings_type", data);
}

static void layer_with_wrong_tile_x_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_double(vtzero::detail::pbf_layer::tile_x, 0.1);

    write_layer("layer_with_wrong_tile_x_type", data);
}

static void layer_with_wrong_tile_y_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_double(vtzero::detail::pbf_layer::tile_y, 0.2);

    write_layer("layer_with_wrong_tile_y_type", data);
}

static void layer_with_wrong_tile_zoom_type() {
    std::string data;
    protozero::pbf_builder<vtzero::detail::pbf_layer> layer_builder{data};

    layer_builder.add_double(vtzero::detail::pbf_layer::tile_zoom, 0.3);

    write_layer("layer_with_wrong_tile_zoom_type", data);
}

static void layer_with_unknown_field() {
    std::string data;
    protozero::pbf_writer layer_builder{data};

    layer_builder.add_uint32(123, 17);

    write_layer("layer_with_unknown_field", data);
}

/****************************************************************************/

static void feature_with_version_2_attributes_field() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint64_t, 2> attrs{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::attributes, attrs.cbegin(), attrs.cend());
    }

    std::string data{wrap_in_layer(2, feature_data)};

    write_layer("feature_with_version_2_attributes_field", data);
}

static void feature_with_version_2_elevations_field() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<int32_t, 2> attrs{{1, 2}};
        feature_builder.add_packed_sint32(vtzero::detail::pbf_feature::elevations, attrs.cbegin(), attrs.cend());
    }

    std::string data{wrap_in_layer(2, feature_data)};

    write_layer("feature_with_version_2_elevations_field", data);
}

static void feature_with_version_2_geom_attributes_field() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint64_t, 2> attrs{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::geometric_attributes, attrs.cbegin(), attrs.cend());
    }

    std::string data{wrap_in_layer(2, feature_data)};

    write_layer("feature_with_version_2_geom_attributes_field", data);
}

static void feature_with_version_2_string_id_field() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_string(vtzero::detail::pbf_feature::string_id, "abc");
    }

    std::string data{wrap_in_layer(2, feature_data)};

    write_layer("feature_with_version_2_string_id_field", data);
}

/****************************************************************************/

static void feature_with_multiple_ids_int_int() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_uint64(vtzero::detail::pbf_feature::id, 123);
        feature_builder.add_uint64(vtzero::detail::pbf_feature::id, 234);
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_ids_int_int", data);
}

static void feature_with_multiple_ids_int_string() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_uint64(vtzero::detail::pbf_feature::id, 123);
        feature_builder.add_string(vtzero::detail::pbf_feature::string_id, "234");
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_ids_int_string", data);
}

static void feature_with_multiple_ids_string_string() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_string(vtzero::detail::pbf_feature::string_id, "123");
        feature_builder.add_string(vtzero::detail::pbf_feature::string_id, "234");
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_ids_string_string", data);
}

/****************************************************************************/

static void feature_with_multiple_tags() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint32_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint32(vtzero::detail::pbf_feature::tags, x.cbegin(), x.cend());
        feature_builder.add_packed_uint32(vtzero::detail::pbf_feature::tags, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_tags", data);
}

static void feature_with_multiple_attributes() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint64_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::attributes, x.cbegin(), x.cend());
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::attributes, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_attributes", data);
}

static void feature_with_multiple_geometric_attributes() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint64_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::geometric_attributes, x.cbegin(), x.cend());
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::geometric_attributes, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_geometric_attributes", data);
}

static void feature_with_multiple_geometries() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint32_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint32(vtzero::detail::pbf_feature::geometry, x.cbegin(), x.cend());
        feature_builder.add_packed_uint32(vtzero::detail::pbf_feature::geometry, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_geometries", data);
}

static void feature_with_multiple_elevations() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<int32_t, 2> x{{1, 2}};
        feature_builder.add_packed_sint32(vtzero::detail::pbf_feature::elevations, x.cbegin(), x.cend());
        feature_builder.add_packed_sint32(vtzero::detail::pbf_feature::elevations, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_elevations", data);
}

static void feature_with_multiple_spline_knots() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint64_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::spline_knots, x.cbegin(), x.cend());
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::spline_knots, x.cbegin(), x.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_multiple_spline_knots", data);
}

/****************************************************************************/

static void feature_with_spline_degree(const uint32_t degree) {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_uint32(vtzero::detail::pbf_feature::spline_degree, degree);
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_spline_degree_" + std::to_string(degree), data);
}

/****************************************************************************/

static void feature_with_tags_and_attributes() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        std::array<uint32_t, 2> x{{1, 2}};
        feature_builder.add_packed_uint32(vtzero::detail::pbf_feature::tags, x.cbegin(), x.cend());
        std::array<uint64_t, 2> y{{1, 2}};
        feature_builder.add_packed_uint64(vtzero::detail::pbf_feature::attributes, y.cbegin(), y.cend());
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_tags_and_attributes", data);
}

static void feature_without_geometry() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_uint64(vtzero::detail::pbf_feature::id, 23);
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_without_geometry", data);
}

static void feature_with_unknown_geometry_type() {
    std::string feature_data;
    {
        protozero::pbf_builder<vtzero::detail::pbf_feature> feature_builder{feature_data};
        feature_builder.add_int32(vtzero::detail::pbf_feature::type, static_cast<int32_t>(vtzero::GeomType::max) + 1);
    }

    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_unknown_geometry_type", data);
}

static void feature_with_unknown_field() {
    std::string feature_data;
    {
        protozero::pbf_writer feature_builder{feature_data};
        feature_builder.add_int32(111, 17);
    }
    std::string data{wrap_in_layer(3, feature_data)};

    write_layer("feature_with_unknown_field", data);
}

/****************************************************************************/

int main(int argc, char *argv[]) {
    if (argc > 1) {
        dir = argv[1];
    } else {
        dir = ".";
    }

    unknown_fields_in_tile();
    only_unknown_fields_in_tile();

    layer_with_invalid_protobuf_type();

    layer_with_version(0);
    layer_with_version(4);

    layer_with_two_double_tables();
    layer_with_two_float_tables();
    layer_with_two_int_tables();

    layer_with_version_2_string_table();
    layer_with_version_2_double_table();
    layer_with_version_2_float_table();
    layer_with_version_2_int_table();
    layer_with_version_2_elevation_scaling();
    layer_with_version_2_attribute_scaling();

    layer_with_attribute_scaling_with_wrong_offset_type();
    layer_with_attribute_scaling_with_wrong_multiplier_type();
    layer_with_attribute_scaling_with_wrong_base_type();
    layer_with_attribute_scaling_with_unknown_field();

    layer_without_name();
    layer_with_empty_name();

    layer_with_zoom_level_100();
    layer_with_tile_x_too_large();
    layer_with_tile_y_too_large();

    layer_with_wrong_version_type();
    layer_with_wrong_name_type();
    layer_with_wrong_features_type();
    layer_with_wrong_keys_type();
    layer_with_wrong_values_type();
    layer_with_wrong_extent_type();
    layer_with_wrong_string_values_type();
    layer_with_wrong_double_values_type();
    layer_with_wrong_float_values_type();
    layer_with_wrong_int_values_type();
    layer_with_wrong_elevation_scaling_type();
    layer_with_wrong_attribute_scalings_type();
    layer_with_wrong_tile_x_type();
    layer_with_wrong_tile_y_type();
    layer_with_wrong_tile_zoom_type();
    layer_with_unknown_field();

    feature_with_version_2_attributes_field();
    feature_with_version_2_elevations_field();
    feature_with_version_2_geom_attributes_field();
    feature_with_version_2_string_id_field();

    feature_with_multiple_ids_int_int();
    feature_with_multiple_ids_int_string();
    feature_with_multiple_ids_string_string();

    feature_with_multiple_tags();
    feature_with_multiple_attributes();
    feature_with_multiple_geometric_attributes();
    feature_with_multiple_geometries();
    feature_with_multiple_elevations();
    feature_with_multiple_spline_knots();

    feature_with_spline_degree(0);
    feature_with_spline_degree(1);
    feature_with_spline_degree(4);

    feature_with_tags_and_attributes();
    feature_without_geometry();
    feature_with_unknown_geometry_type();
    feature_with_unknown_field();
}

/****************************************************************************/
