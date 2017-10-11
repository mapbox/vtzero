
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/vector_tile.hpp>

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

static std::string open_tile(const std::string& path) {
    const auto fixtures_dir = std::getenv("FIXTURES_DIR");
    if (fixtures_dir == nullptr) {
        std::cerr << "Set FIXTURES_DIR environment variable to the directory where the mvt fixtures are!\n";
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

}; // struct point_handler

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

}; // struct linestring_handler

struct polygon_handler {

    std::vector<std::vector<vtzero::point>> data;

    void ring_begin(uint32_t count) {
        data.emplace_back();
        data.back().reserve(count);
    }

    void ring_point(const vtzero::point point) {
        data.back().push_back(point);
    }

    void ring_end(bool /*dummy*/) const noexcept {
    }

}; // struct polygon_handler

// ---------------------------------------------------------------------------

vtzero::feature check_layer(const vtzero::vector_tile& tile) {
    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "hello");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.size() == 1);

    return *layer.begin();
}

// ---------------------------------------------------------------------------

TEST_CASE("MVT test 001: Empty tile") {
    std::string buffer{open_tile("001/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.empty());
    REQUIRE(tile.size() == 0); // NOLINT clang-tidy: readability-container-size-empty

    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 0);
}

TEST_CASE("MVT test 002: Tile with single point feature without id") {
    std::string buffer{open_tile("002/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto num_layers = std::distance(tile.begin(), tile.end());
    REQUIRE(num_layers == 1);

    const auto feature = check_layer(tile);

    REQUIRE_FALSE(feature.has_id());
    REQUIRE(feature.id() == 0);
    REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{25, 17}};
    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 003: Tile with single point with missing geometry type") {
    std::string buffer{open_tile("003/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);
    REQUIRE(feature.has_id());
    REQUIRE(feature.id() == 1);
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
}

TEST_CASE("MVT test 004: Tile with single point with missing geometry") {
    std::string buffer{open_tile("004/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE_THROWS_AS(check_layer(tile), const vtzero::format_exception&);
}

TEST_CASE("MVT test 005: Tile with single point with broken tags array") {
    std::string buffer{open_tile("005/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());
    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 006: Tile with single point with invalid GeomType") {
    std::string buffer{open_tile("006/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 007: Layer version as string instead of as an int") {
    std::string buffer{open_tile("007/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    REQUIRE_THROWS_AS(tile[0], const vtzero::format_exception&);
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

    vtzero::property_value_view v{layer.value(0)};
    REQUIRE_THROWS_AS(v.type(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 011: Tile layer value is encoded as unknown type") {
    std::string buffer{open_tile("011/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    vtzero::property_value_view v{layer.value(0)};
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
    REQUIRE_THROWS_AS(*tile.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 014: Tile layer without a name") {
    std::string buffer{open_tile("014/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    REQUIRE_THROWS_AS(tile[0], const vtzero::format_exception&);
}

TEST_CASE("MVT test 015: Two layers with the same name") {
    std::string buffer{open_tile("015/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 2);

    for (const auto layer : tile) {
        REQUIRE(layer.name() == "hello");
    }

    const auto layer = tile["hello"];
    REQUIRE(layer.name() == "hello");
}

TEST_CASE("MVT test 016: Valid unknown geometry") {
    std::string buffer{open_tile("016/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
}

TEST_CASE("MVT test 017: Valid point geometry") {
    std::string buffer{open_tile("017/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.has_id());
    REQUIRE(feature.id() == 1);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    const std::vector<vtzero::point> result = {{25, 17}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 018: Valid linestring geometry") {
    std::string buffer = open_tile("018/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 019: Valid polygon geometry") {
    std::string buffer = open_tile("019/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::POLYGON);

    polygon_handler handler;
    vtzero::decode_polygon_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{3, 6}, {8,12}, {20, 34}, {3, 6}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 020: Valid multipoint geometry") {
    std::string buffer = open_tile("020/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::POINT);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    const std::vector<vtzero::point> result = {{5, 7}, {3,2}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 021: Valid multilinestring geometry") {
    std::string buffer = open_tile("021/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::LINESTRING);

    linestring_handler handler;
    vtzero::decode_linestring_geometry(feature.geometry(), true, handler);

    const std::vector<std::vector<vtzero::point>> result = {{{2, 2}, {2,10}, {10, 10}}, {{1,1}, {3, 5}}};

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 022: Valid multipolygon geometry") {
    std::string buffer = open_tile("022/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto feature = check_layer(tile);

    REQUIRE(feature.geometry_type() == vtzero::GeomType::POLYGON);

    polygon_handler handler;
    vtzero::decode_polygon_geometry(feature.geometry(), false, handler);

    const std::vector<std::vector<vtzero::point>> result = {
        {{0, 0}, {10, 0}, {10, 10}, {0,10}, {0, 0}},
        {{11, 11}, {20, 11}, {20, 20}, {11, 20}, {11, 11}},
        {{13, 13}, {13, 17}, {17, 17}, {17, 13}, {13, 13}}
    };

    REQUIRE(handler.data == result);
}

TEST_CASE("MVT test 023: Invalid layer: missing layer name") {
    std::string buffer = open_tile("023/tile.mvt");
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    REQUIRE_THROWS_AS(*tile.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 024: Missing layer version") {
    std::string buffer = open_tile("024/tile.mvt");
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.version() == 1);
}

TEST_CASE("MVT test 025: Layer without features") {
    std::string buffer = open_tile("025/tile.mvt");
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.empty());
    REQUIRE(layer.size() == 0); // NOLINT clang-tidy: readability-container-size-empty
}

TEST_CASE("MVT test 026: Extra value type") {
    std::string buffer = open_tile("026/tile.mvt");
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.empty());

    const auto table = layer.value_table();
    REQUIRE(table.size() == 1);

    const auto pvv = table[0];
    REQUIRE(pvv.valid());
    REQUIRE_THROWS_AS(pvv.type(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 027: Layer with unused bool property value") {
    std::string buffer{open_tile("027/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 0); // NOLINT clang-tidy: readability-container-size-empty

    const auto& vtab = layer.value_table();
    REQUIRE(vtab.size() == 1);
    REQUIRE(vtab[0].bool_value());
}

TEST_CASE("MVT test 030: Two geometry fields") {
    std::string buffer = open_tile("030/tile.mvt");
    vtzero::vector_tile tile{buffer};

    const auto layer = *tile.begin();
    REQUIRE_FALSE(layer.empty());

    REQUIRE_THROWS_AS(*layer.begin(), const vtzero::format_exception&);
}

TEST_CASE("MVT test 032: Layer with single feature with string property value") {
    std::string buffer{open_tile("032/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE_FALSE(tile.empty());
    REQUIRE(tile.size() == 1);

    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().string_value() == "i am a string value");

    const auto it = feature.begin();
    REQUIRE(it.get_index_pair().first.value() == 0);
    REQUIRE(it.get_index_pair().second.value() == 0);
}

TEST_CASE("MVT test 033: Layer with single feature with float property value") {
    std::string buffer{open_tile("033/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().float_value() == Approx(3.1));
}

TEST_CASE("MVT test 034: Layer with single feature with double property value") {
    std::string buffer{open_tile("034/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().double_value() == Approx(1.23));
}

TEST_CASE("MVT test 035: Layer with single feature with int property value") {
    std::string buffer{open_tile("035/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().int_value() == 6);
}

TEST_CASE("MVT test 036: Layer with single feature with uint property value") {
    std::string buffer{open_tile("036/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().uint_value() == 87948);
}

TEST_CASE("MVT test 037: Layer with single feature with sint property value") {
    std::string buffer{open_tile("037/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);

    const auto prop = *feature.begin();
    REQUIRE(prop.key() == "key1");
    REQUIRE(prop.value().sint_value() == 87948);
}

TEST_CASE("MVT test 038: Layer with all types of property value") {
    std::string buffer{open_tile("038/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();

    const auto& vtab = layer.value_table();
    REQUIRE(vtab.size() == 7);
    REQUIRE(vtab[0].string_value() == "ello");
    REQUIRE(vtab[1].bool_value());
    REQUIRE(vtab[2].int_value() == 6);
    REQUIRE(vtab[3].double_value() == Approx(1.23));
    REQUIRE(vtab[4].float_value() == Approx(3.1));
    REQUIRE(vtab[5].sint_value() == 87948);
    REQUIRE(vtab[6].uint_value() == 87948);

    REQUIRE_THROWS_AS(vtab[0].bool_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[0].int_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[0].double_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[0].float_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[0].sint_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[0].uint_value(), const vtzero::type_exception&);
    REQUIRE_THROWS_AS(vtab[1].string_value(), const vtzero::type_exception&);
}

TEST_CASE("MVT test 039: Default values are actually encoded in the tile") {
    std::string buffer{open_tile("039/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = tile[0];
    REQUIRE(layer.version() == 1);
    REQUIRE(layer.name() == "hello");
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 0);
    REQUIRE(feature.geometry_type() == vtzero::GeomType::UNKNOWN);
    REQUIRE(feature.empty());
}

TEST_CASE("MVT test 040: Feature has tags that point to non-existent Key in the layer.") {
    std::string buffer{open_tile("040/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = tile[0];
    REQUIRE(layer.size() == 1);
    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);
    REQUIRE_THROWS_AS(*feature.begin(), const std::out_of_range&);
}

TEST_CASE("MVT test 041: Tags encoded as floats instead of as ints") {
    std::string buffer{open_tile("041/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = tile[0];
    REQUIRE(layer.size() == 1);
    const auto feature = *layer.begin();
    REQUIRE_THROWS_AS(*feature.begin(), const std::out_of_range&);
}

TEST_CASE("MVT test 042: Feature has tags that point to non-existent Value in the layer.") {
    std::string buffer{open_tile("042/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = tile[0];
    REQUIRE(layer.size() == 1);
    const auto feature = *layer.begin();
    REQUIRE(feature.size() == 1);
    REQUIRE_THROWS_AS(*feature.begin(), const std::out_of_range&);
}

TEST_CASE("MVT test 043: A layer with six points that all share the same key but each has a unique value.") {
    std::string buffer{open_tile("043/tile.mvt")};
    vtzero::vector_tile tile{buffer};

    REQUIRE(tile.size() == 1);
    const auto layer = tile[0];
    REQUIRE(layer.size() == 6);
}

