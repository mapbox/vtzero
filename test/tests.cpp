
#include <vtzero/geometry.hpp>
#include <vtzero/builder.hpp>
#include <vtzero/vector_tile.hpp>

#include <catch.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

static std::string open_tile(const std::string& path) {
    std::ifstream stream{std::string{"test/mvt-fixtures/fixtures/"} + path.c_str(), std::ios_base::in|std::ios_base::binary};
    if (!stream.is_open()) {
        throw std::runtime_error{"could not open: '" + path + "'"};
    }
    std::string message{std::istreambuf_iterator<char>(stream.rdbuf()),
                        std::istreambuf_iterator<char>()};
    stream.close();
    return message;
}

struct point_handler {

    std::vector<vtzero::point> data;

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

struct linestring_handler {

    std::vector<std::vector<vtzero::point>> data;

    void linestring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void linestring_point(const vtzero::point point) {
        data.back().push_back(point);
    }

    void linestring_end() const noexcept {
    }

};

struct polygon_handler {

    std::vector<std::vector<vtzero::point>> data;

    void ring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void ring_point(const vtzero::point point) {
        data.back().push_back(point);
    }

    void ring_end(bool) const noexcept {
    }

};

vtzero::feature check_layer(vtzero::vector_tile& tile) {
    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "hello");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.get_feature_count() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 0);

    return feature;
}

TEST_CASE("Empty tile") {
    std::string buffer{open_tile("001/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 0);
}

TEST_CASE("Read Feature-single-point.mvt") {
    std::string buffer{open_tile("002/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{25, 17}};
    REQUIRE(handler.data == result);
}

#if 0
TEST_CASE("Read Feature-single-multipoint.mvt") {
    std::string buffer = open_tile("valid/Feature-single-multipoint.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{5, 7}, {3,2}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Read Feature-single-linestring.mvt") {
    std::string buffer = open_tile("valid/Feature-single-linestring.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Read Feature-single-multilinestring.mvt") {
    std::string buffer = open_tile("valid/Feature-single-multilinestring.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}, {{1,1}, {3, 5}}};
    REQUIRE(handler.data == result);
}

TEST_CASE("Read Feature-single-polygon.mvt") {
    std::string buffer = open_tile("valid/Feature-single-polygon.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POLYGON);

    polygon_handler handler;
    vtzero::decode_polygon_geometry(feature.geometry(), true, handler);

    std::vector<std::vector<vtzero::point>> result = {{{3, 6}, {8,12}, {20, 34}, {3, 6}}};
    REQUIRE(handler.data == result);
}
#endif

TEST_CASE("Read feature with unknown GeomType") {
    std::string buffer{open_tile("006/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 1);

    const auto layer = *tile.begin();

    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&);
}

struct mypoint {
    int64_t p1;
    int64_t p2;
};

/*inline vtzero::point create_point(mypoint p) noexcept {
    return {static_cast<int32_t>(p.p1),
            static_cast<int32_t>(p.p2)};
}*/

namespace vtzero {

    template <>
    point create_point<::mypoint>(::mypoint p) noexcept {
        return {static_cast<int32_t>(p.p1),
                static_cast<int32_t>(p.p2)};
    }
}

TEST_CASE("Point builder") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder, 17};

    SECTION("add point using coordinates") {
        fbuilder.add_point(10, 20);
    }

    SECTION("add point using vtzero::point") {
        fbuilder.add_point(vtzero::point{10, 20});
    }

    SECTION("add point using mypoint") {
        fbuilder.add_point(mypoint{10, 20});
    }

    fbuilder.commit();

    std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.get_feature_count() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 17);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

