
#include "utils.hpp"

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>

#include <unordered_map>

int main() {
    vtzero::tile_builder tile;
    vtzero::layer_builder layer_points{tile, "points"};
    vtzero::layer_builder layer_lines{tile, "lines"};
    vtzero::layer_builder layer_polygons{tile, "polygons"};

    vtzero::key_index<std::unordered_map> idx{layer_points};

    {
        vtzero::point_feature_builder feature{layer_points, 1 /* id */};
        feature.add_points(1);
        feature.set_point(10, 10);
        feature.add_property("foo", "bar");
        feature.add_property("x", "y");
        feature.rollback();
    }

    const auto some = idx("some");

    {
        vtzero::point_feature_builder feature{layer_points, 2 /* id */};
        feature.add_point(20, 20);
        feature.add_property(some, "attr");
    }
    {
        vtzero::point_feature_builder feature{layer_points, 3 /* id */};
        feature.add_point(20, 20);
        feature.add_property(idx("some"), "attr");
    }

    {
        vtzero::point_feature_builder feature{layer_points, 4 /* id */};
        feature.add_point(20, 20);
        feature.add_property(idx("some"), "otherattr");
    }


    vtzero::point_feature_builder feature1{layer_points, 5 /* id */};
    feature1.add_point(vtzero::point{20, 20});
    feature1.add_property("otherkey", "attr");
    feature1.commit();

    vtzero::value_index<vtzero::sint_value_type, int32_t, std::unordered_map> maxspeed_index{layer_lines};
    {
        vtzero::line_string_feature_builder feature{layer_lines, 6 /* id */};
        feature.add_linestring(3);
        feature.set_point(10, 10);
        feature.set_point(10, 20);
        feature.set_point(vtzero::point{20, 20});
        std::vector<vtzero::point> points = {{11, 11}, {12, 13}};
        feature.add_linestring(points.begin(), points.end());
        feature.add_property("highway", "primary");
        feature.add_property(std::string{"maxspeed"}, maxspeed_index(50));
    }

    {
        vtzero::polygon_feature_builder feature{layer_polygons, 7 /* id */};
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
        feature.add_property("natural", "wood");
        feature.add_property("number_of_trees", vtzero::sint_value_type{23402752});
    }

    const auto data = tile.serialize();
    write_data_to_file(data, "test.mvt");
}

