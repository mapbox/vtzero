
#include <vtzero/builder.hpp>

#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    vtzero::tile_builder tile;
    vtzero::layer_builder layer_points{tile, "points"};
    vtzero::layer_builder layer_lines{tile, "lines"};
    vtzero::layer_builder layer_polygons{tile, "polygons"};

    {
        vtzero::point_feature_builder feature{layer_points, 1 /* id */};
        feature.add_points(1);
        feature.set_point(10, 10);
        feature.add_tag("foo", "bar");
        feature.add_tag("x", "y");
        feature.rollback();
    }

    {
        vtzero::point_feature_builder feature{layer_points, 2 /* id */};
        feature.add_point(20, 20);
        feature.add_tag("some", "attr");
    }
    {
        vtzero::point_feature_builder feature{layer_points, 3 /* id */};
        feature.add_point(20, 20);
        feature.add_tag("some", "attr");
    }

    {
        vtzero::point_feature_builder feature{layer_points, 4 /* id */};
        feature.add_point(20, 20);
        feature.add_tag("some", "otherattr");
    }


    vtzero::point_feature_builder feature{layer_points, 5 /* id */};
    feature.add_point(vtzero::point{20, 20});
    feature.add_tag("otherkey", "attr");
    feature.commit();

    {
        vtzero::line_string_feature_builder feature{layer_lines, 6 /* id */};
        feature.add_linestring(3);
        feature.set_point(10, 10);
        feature.set_point(10, 20);
        feature.set_point(vtzero::point{20, 20});
        std::vector<vtzero::point> points = {{11, 11}, {12, 13}};
        feature.add_linestring(points.begin(), points.end());
        feature.add_tag("highway", "primary");
        feature.add_tag(std::string{"maxspeed"}, vtzero::sint_value(50));
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
        feature.add_tag("natural", "wood");
    }

    const auto data = tile.serialize();
    const int fd = ::open("test.mvt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd > 0);

    const auto len = ::write(fd, data.c_str(), data.size());
    assert(static_cast<size_t>(len) == data.size());
}

