/*****************************************************************************

  Example program for vtzero library.

  vtzero-create - Create a vector tile

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/builder_helper.hpp>
#include <vtzero/index.hpp>

#include <clara.hpp>

#include <cstdint>
#include <iostream>
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
        vtzero::point_feature_builder<2> fbuilder{layer_points};
        fbuilder.set_integer_id(1);
        fbuilder.add_points(1);
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.add_scalar_attribute("foo", "bar");
        fbuilder.add_scalar_attribute("x", "y");
        fbuilder.rollback();
    }

    const auto some = idx("some");

    {
        vtzero::point_feature_builder<2> fbuilder{layer_points};
        fbuilder.set_integer_id(2);
        fbuilder.add_point(20, 20);
        fbuilder.add_scalar_attribute(some, "attr");
        fbuilder.commit();
    }
    {
        vtzero::point_feature_builder<2> fbuilder{layer_points};
        fbuilder.set_integer_id(3);
        fbuilder.add_point(20, 20);
        fbuilder.add_scalar_attribute(idx("some"), "attr");
        fbuilder.commit();
    }

    {
        vtzero::point_feature_builder<2> fbuilder{layer_points};
        fbuilder.set_integer_id(4);
        fbuilder.add_point(20, 20);
        fbuilder.add_scalar_attribute(idx("some"), "otherattr");
        fbuilder.commit();
    }


    vtzero::point_feature_builder<2> fbuilder1{layer_points};
    fbuilder1.set_integer_id(5);
    fbuilder1.add_point(vtzero::point_2d{20, 20});
    fbuilder1.add_scalar_attribute("otherkey", "attr");
    fbuilder1.commit();

    {
        vtzero::linestring_feature_builder<2> fbuilder{layer_lines};
        fbuilder.set_integer_id(6);
        fbuilder.add_linestring(3);
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.set_point(vtzero::point_2d{10, 20});
        fbuilder.set_point(vtzero::point_2d{20, 20});
        std::vector<vtzero::point_2d> points = {{11, 11}, {12, 13}};
        vtzero::add_linestring_from_container(points, fbuilder);
        fbuilder.add_scalar_attribute("highway", "primary");
        fbuilder.add_scalar_attribute(std::string{"maxspeed"}, 50);
        fbuilder.commit();
    }

    {
        vtzero::polygon_feature_builder<2> fbuilder{layer_polygons};
        fbuilder.set_integer_id(7);
        fbuilder.add_ring(5);
        fbuilder.set_point(vtzero::point_2d{0, 0});
        fbuilder.set_point(vtzero::point_2d{10, 0});
        fbuilder.set_point(vtzero::point_2d{10, 10});
        fbuilder.set_point(vtzero::point_2d{0, 10});
        fbuilder.set_point(vtzero::point_2d{0, 0});
        fbuilder.add_ring(4);
        fbuilder.set_point(vtzero::point_2d{3, 3});
        fbuilder.set_point(vtzero::point_2d{3, 5});
        fbuilder.set_point(vtzero::point_2d{5, 5});
        fbuilder.close_ring();
        fbuilder.add_scalar_attribute("natural", "wood");
        fbuilder.add_scalar_attribute("number_of_trees", 23402752);
        fbuilder.commit();
    }

    const auto data = tile.serialize();
    write_data_to_file(data, "test.mvt");
}

