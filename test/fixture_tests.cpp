
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
    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "hello");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();

    return feature;
}

TEST_CASE("MVT test 001: Empty tile") {
    std::string buffer{open_tile("001/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.empty());
    REQUIRE(tile.size() == 0);
    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 0);
}

TEST_CASE("MVT test 002: Tile with single point feature without id") {
    std::string buffer{open_tile("002/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);
    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 1);

    const auto feature = check_layer(tile);

    REQUIRE_FALSE(feature.has_id());
    REQUIRE(feature.id() == 0);
    REQUIRE(feature.type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{25, 17}};
    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 003: Tile with single point with missing geometry type") {
    std::string buffer{open_tile("003/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);

    const auto feature = check_layer(tile);
    REQUIRE(feature.has_id());
    REQUIRE(feature.id() == 1);
    REQUIRE(feature.type() == vtzero::GeomType::UNKNOWN);
}

TEST_CASE("MVT test 004: Tile with single point with missing geometry") {
    std::string buffer{open_tile("004/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);

    REQUIRE_THROWS_AS(check_layer(tile), const vtzero::format_exception&);
}

TEST_CASE("MVT test 005: Tile with single point with broken tags array") {
    std::string buffer{open_tile("005/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());
    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 1);

//    REQUIRE_THROWS_AS(feature.tags(layer).size(), const vtzero::format_exception&); XXX
    auto it = feature.tags(layer).begin();
    REQUIRE(it != feature.tags(layer).end());
    REQUIRE_THROWS_AS(it.size(), const vtzero::format_exception&);
    REQUIRE_THROWS_AS(*it, const vtzero::format_exception&);
    REQUIRE_THROWS_AS(++it, const vtzero::format_exception&);
}

TEST_CASE("MVT test 006: Tile with single point with invalid GeomType") {
    std::string buffer{open_tile("006/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 007: Tile with key in table encoded as int") {
    std::string buffer{open_tile("007/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 1);

    auto it = feature.tags(layer).begin();
    (void)it;
/*    REQUIRE_THROWS_AS((*it).key(), const vtzero::format_exception&); XXX broken test fixture?
    */
}

TEST_CASE("MVT test 008: Tile layer extent encoded as string") {
    std::string buffer{open_tile("008/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    REQUIRE_THROWS_AS(*tile.begin(), const vtzero::format_exception&);
}

// missing 009

TEST_CASE("MVT test 010: Tile layer value is encoded as int, but pretends to be string") {
    std::string buffer{open_tile("010/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    vtzero::value_view v{layer.value(0)};
    REQUIRE_THROWS_AS(v.type(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 011: Tile layer value is encoded as unknown type") {
    std::string buffer{open_tile("011/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    vtzero::value_view v{layer.value(0)};
    REQUIRE_THROWS_AS(v.type(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 012: Unknown layer version") {
    std::string buffer{open_tile("012/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    REQUIRE_THROWS_AS(*tile.begin(), const vtzero::version_exception&);
}

TEST_CASE("MVT test 013: Tile with key in table encoded as int") {
    std::string buffer{open_tile("013/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 1);

/*    auto it = feature.tags(layer).begin();
    REQUIRE_THROWS_AS((*it).key(), const vtzero::format_exception&); XXX*/
}

TEST_CASE("MVT test 014: Tile layer without a name") {
    std::string buffer{open_tile("014/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE(tile.size() == 1);

    REQUIRE_THROWS_AS(*tile.begin(), const vtzero::format_exception&);
}

// XXX 015: we do not test for this, because it would be expensive
//          extra check?

TEST_CASE("MVT test 017: Valid point geometry") {
    std::string buffer{open_tile("017/tile.mvt")};
    vtzero::vector_tile tile{buffer};
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);
    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 1);

    const auto feature = check_layer(tile);

    REQUIRE(feature.has_id());
    REQUIRE(feature.id() == 1);

    REQUIRE(feature.type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    const std::vector<vtzero::point> result = {{25, 17}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 018: Valid linestring geometry") {
    std::string buffer = open_tile("018/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 019: Valid polygon geometry") {
    std::string buffer = open_tile("019/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POLYGON);

    polygon_handler handler;
    vtzero::decode_polygon_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{3, 6}, {8,12}, {20, 34}, {3, 6}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 020: Valid multipoint geometry") {
    std::string buffer = open_tile("020/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    const std::vector<vtzero::point> result = {{5, 7}, {3,2}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 021: Valid multilinestring geometry") {
    std::string buffer = open_tile("021/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}, {{1,1}, {3, 5}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 022: Valid multipolygon geometry") {
    std::string buffer = open_tile("022/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.type() == vtzero::GeomType::POLYGON);

    polygon_handler handler;
    vtzero::decode_polygon_geometry(feature.geometry(), false, handler);

    const std::vector<std::vector<vtzero::point>> result = {
        {{0, 0}, {10, 0}, {10, 10}, {0,10}, {0, 0}},
        {{11, 11}, {20, 11}, {20, 20}, {11, 20}, {11, 11}},
        {{13, 13}, {13, 17}, {17, 17}, {17, 13}, {13, 13}}
    };

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 030: Two geometry fields") {
    std::string buffer = open_tile("030/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&); // XXX see https://github.com/mapbox/mvt-fixtures/issues/36
}

