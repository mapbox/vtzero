/*****************************************************************************

  Example program for vtzero library.

  vtzero-create - Create a vector tile

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>

#include <clara.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

int main(int argc, char* argv[]) {
    bool help;
    bool version_2;

    const auto cli
        = clara::Opt(version_2)
            ["-2"]
            ("write version 2 tile")
        | clara::Help(help);

    const auto result = cli.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << '\n';
        return 1;
    }

    if (help) {
        std::cout << cli
                  << "\nCreate vector tile called 'test.mvt'.\n";
        return 0;
    }

    uint32_t version = version_2 ? 2 : 3;

    vtzero::tile_builder tile;
    vtzero::layer_builder layer_points{tile, "points", version};
    vtzero::layer_builder layer_lines{tile, "lines", version};
    vtzero::layer_builder layer_polygons{tile, "polygons", version};

    vtzero::key_index<std::unordered_map> idx{layer_points};

    {
        vtzero::point_2d_feature_builder feature{layer_points};
        feature.set_integer_id(1);
        feature.add_points(1);
        feature.set_point(10, 10);
        feature.add_scalar_attribute("foo", "bar");
        feature.add_scalar_attribute("x", "y");
        feature.rollback();
    }

    const auto some = idx("some");

    {
        vtzero::point_2d_feature_builder feature{layer_points};
        feature.set_integer_id(2);
        feature.add_point(20, 20);
        feature.add_scalar_attribute(some, "attr");
        feature.commit();
    }
    {
        vtzero::point_2d_feature_builder feature{layer_points};
        feature.set_integer_id(3);
        feature.add_point(20, 20);
        feature.add_scalar_attribute(idx("some"), "attr");
        feature.commit();
    }

    {
        vtzero::point_2d_feature_builder feature{layer_points};
        feature.set_integer_id(4);
        feature.add_point(20, 20);
        feature.add_scalar_attribute(idx("some"), "otherattr");
        feature.commit();
    }


    vtzero::point_2d_feature_builder feature1{layer_points};
    feature1.set_integer_id(5);
    feature1.add_point(vtzero::point{20, 20});
    feature1.add_scalar_attribute("otherkey", "attr");
    feature1.commit();

    {
        vtzero::linestring_2d_feature_builder feature{layer_lines};
        feature.set_integer_id(6);
        feature.add_linestring(3);
        feature.set_point(10, 10);
        feature.set_point(10, 20);
        feature.set_point(vtzero::point{20, 20});
        std::vector<vtzero::point> points = {{11, 11}, {12, 13}};
        feature.add_linestring_from_container(points);
        feature.add_scalar_attribute("highway", "primary");
        feature.add_scalar_attribute(std::string{"maxspeed"}, 50);
        feature.commit();
    }

    {
        vtzero::polygon_2d_feature_builder feature{layer_polygons};
        feature.set_integer_id(7);
        feature.add_ring(5);
        feature.set_point(0, 0);
        feature.set_point(10, 0);
        feature.set_point(10, 10);
        feature.set_point(0, 10);
        feature.set_point(0, 0);
        feature.add_ring(4);
        feature.set_point(3, 3);
        feature.set_point(3, 5);
        feature.set_point(5, 5);
        feature.close_ring();
        feature.add_scalar_attribute("natural", "wood");
        feature.add_scalar_attribute("number_of_trees", 23402752);
        feature.commit();
    }

    const auto data = tile.serialize();
    write_data_to_file(data, "test.mvt");
}

