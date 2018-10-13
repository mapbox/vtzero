
#include <test.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/index.hpp>
#include <vtzero/output.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

template <typename T>
struct movable_not_copyable {
    constexpr static bool value = !std::is_copy_constructible<T>::value &&
                                  !std::is_copy_assignable<T>::value    &&
                                   std::is_nothrow_move_constructible<T>::value &&
                                   std::is_nothrow_move_assignable<T>::value;
};

static_assert(movable_not_copyable<vtzero::tile_builder>::value, "tile_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::point_2d_feature_builder>::value, "point_feature_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::linestring_2d_feature_builder>::value, "linestring_feature_builder should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<vtzero::polygon_2d_feature_builder>::value, "polygon_feature_builder should be nothrow movable, but not copyable");

TEST_CASE("Create tile from existing layers") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    SECTION("add_existing_layer(layer)") {
        while (auto layer = tile.next_layer()) {
            tbuilder.add_existing_layer(layer);
        }
    }

    SECTION("add_existing_layer(data_view)") {
        while (auto layer = tile.next_layer()) {
            tbuilder.add_existing_layer(layer.data());
        }
    }

    const std::string data = tbuilder.serialize();

    REQUIRE(data == buffer);
}

TEST_CASE("Create layer based on existing layer") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};
    const auto layer = tile.get_layer_by_name("place_label");

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, layer};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};
    fbuilder.set_integer_id(42);
    fbuilder.add_point(10, 20);
    fbuilder.commit();

    const std::string data = tbuilder.serialize();
    vtzero::vector_tile new_tile{data};
    const auto new_layer = new_tile.next_layer();
    REQUIRE(std::string(new_layer.name()) == "place_label");
    REQUIRE(new_layer.version() == 1);
    REQUIRE(new_layer.extent() == 4096);
}

TEST_CASE("Create layer and add keys/values") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "name"};

    const auto ki1 = lbuilder.add_key_without_dup_check("key1");
    const auto ki2 = lbuilder.add_key("key2");
    const auto ki3 = lbuilder.add_key("key1");

    REQUIRE(ki1 != ki2);
    REQUIRE(ki1 == ki3);

    const auto vi1 = lbuilder.add_value_without_dup_check(vtzero::encoded_property_value{"value1"});
    vtzero::encoded_property_value value2{"value2"};
    const auto vi2 = lbuilder.add_value_without_dup_check(vtzero::property_value{value2.data()});

    const auto vi3 = lbuilder.add_value(vtzero::encoded_property_value{"value1"});
    const auto vi4 = lbuilder.add_value(vtzero::encoded_property_value{19});
    const auto vi5 = lbuilder.add_value(vtzero::encoded_property_value{19.0});
    const auto vi6 = lbuilder.add_value(vtzero::encoded_property_value{22});
    vtzero::encoded_property_value nineteen{19};
    const auto vi7 = lbuilder.add_value(vtzero::property_value{nineteen.data()});

    REQUIRE(vi1 != vi2);
    REQUIRE(vi1 == vi3);
    REQUIRE(vi1 != vi4);
    REQUIRE(vi1 != vi5);
    REQUIRE(vi1 != vi6);
    REQUIRE(vi4 != vi5);
    REQUIRE(vi4 != vi6);
    REQUIRE(vi4 == vi7);
}

TEST_CASE("Create layer and add scalings") {
    vtzero::scaling scaling_elev{11, 2.2, 3.3};

    vtzero::scaling scaling0{0, 1.0, 0.0};
    vtzero::scaling scaling1{1, 2.0, 1.0};
    vtzero::scaling scaling2{2, 3.0, 2.0};

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "name", 3};

    { // we need to add a feature, otherwise the layer will not be serialized
        vtzero::point_feature_builder<3, true> fbuilder{lbuilder};
        fbuilder.add_point(vtzero::unscaled_point{});
        fbuilder.commit();
    }

    REQUIRE(lbuilder.elevation_scaling() == vtzero::scaling{});
    lbuilder.set_elevation_scaling(scaling_elev);
    REQUIRE(lbuilder.elevation_scaling() == scaling_elev);

    const auto index0 = lbuilder.add_attribute_scaling(scaling0);
    REQUIRE(index0.value() == 0);
    const auto index1 = lbuilder.add_attribute_scaling(scaling1);
    REQUIRE(index1.value() == 1);
    const auto index2 = lbuilder.add_attribute_scaling(scaling2);
    REQUIRE(index2.value() == 2);

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    auto layer = tile.next_layer();
    REQUIRE(layer);
    REQUIRE(layer.name() == "name");
    REQUIRE(layer.version() == 3);
    REQUIRE(layer.num_features() == 1);

    REQUIRE(layer.elevation_scaling() == scaling_elev);
    REQUIRE(layer.num_attribute_scalings() == 3);
    REQUIRE(layer.attribute_scaling(vtzero::index_value{0}) == scaling0);
    REQUIRE(layer.attribute_scaling(vtzero::index_value{1}) == scaling1);
    REQUIRE(layer.attribute_scaling(vtzero::index_value{2}) == scaling2);
    REQUIRE_THROWS_AS(layer.attribute_scaling(vtzero::index_value{3}), const std::out_of_range&);
}

TEST_CASE("Committing a feature succeeds after a geometry was added") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    { // explicit commit after geometry
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 10);
        fbuilder.commit();
    }

    { // explicit commit after properties
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(2);
        fbuilder.add_point(10, 10);
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
        fbuilder.commit();
    }

    { // extra commits or rollbacks are okay but no other calls
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(3);
        fbuilder.add_point(10, 10);
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
        fbuilder.commit();

        SECTION("superfluous commit()") {
            fbuilder.commit();
        }
        SECTION("superfluous rollback()") {
            fbuilder.rollback();
        }

        REQUIRE_THROWS_AS(fbuilder.set_integer_id(10), const assert_error&);
        REQUIRE_THROWS_AS(fbuilder.add_point(20, 20), const assert_error&);
        REQUIRE_THROWS_AS(fbuilder.add_property("x", "y"), const assert_error&);
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};
    auto layer = tile.next_layer();

    uint64_t n = 1;
    while (auto feature = layer.next_feature()) {
        REQUIRE(feature.id() == n++);
    }

    REQUIRE(n == 4);
}

TEST_CASE("Committing a feature fails with assert if no geometry was added") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    SECTION("explicit immediate commit") {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        REQUIRE_THROWS_AS(fbuilder.commit(), const assert_error&);
    }

    SECTION("explicit commit after setting id") {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(2);
        REQUIRE_THROWS_AS(fbuilder.commit(), const assert_error&);
    }
}

TEST_CASE("String ids are not allowed in version 2 tiles") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};
    REQUIRE_THROWS_AS(fbuilder.set_string_id("foo"), const assert_error&);
}

TEST_CASE("String ids are okay in version 3 tiles") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};
    fbuilder.set_string_id("foo");
    fbuilder.add_point(10, 10);
    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};
    auto layer = tile.next_layer();

    auto feature = layer.next_feature();
    REQUIRE_FALSE(feature.has_integer_id());
    REQUIRE(feature.has_string_id());
    REQUIRE(feature.string_id() == "foo");
}

TEST_CASE("Create layer with x/y/zoom/extent") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test", 3, vtzero::tile{5, 3, 12, 8192}};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};
    fbuilder.set_string_id("foo");
    fbuilder.add_point(10, 10);
    fbuilder.commit();

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};
    auto layer = tile.next_layer();

    REQUIRE(layer.get_tile().x() == 5);
    REQUIRE(layer.get_tile().y() == 3);
    REQUIRE(layer.get_tile().zoom() == 12);
    REQUIRE(layer.extent() == 8192);
}

TEST_CASE("Rollback feature") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(1);
        fbuilder.add_point(10, 10);
        fbuilder.commit();
    }

    { // immediate rollback
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(2);
        fbuilder.rollback();
    }

    { // rollback after setting id
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(3);
        fbuilder.rollback();
    }

    { // rollback after geometry
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(4);
        fbuilder.add_point(20, 20);
        fbuilder.rollback();
    }

    { // rollback after properties
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(5);
        fbuilder.add_point(20, 20);
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
        fbuilder.rollback();
    }

    { // implicit rollback after geometry
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(6);
        fbuilder.add_point(10, 10);
    }

    { // implicit rollback after properties
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(7);
        fbuilder.add_point(10, 10);
        fbuilder.add_property("foo", vtzero::encoded_property_value{"bar"});
    }

    {
        vtzero::point_2d_feature_builder fbuilder{lbuilder};
        fbuilder.set_integer_id(8);
        fbuilder.add_point(30, 30);
        fbuilder.commit();
    }

    const std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};
    auto layer = tile.next_layer();

    auto feature = layer.next_feature();
    REQUIRE(feature.has_integer_id());
    REQUIRE_FALSE(feature.has_string_id());
    REQUIRE(feature.id() == 1);

    feature = layer.next_feature();
    REQUIRE(feature.has_integer_id());
    REQUIRE_FALSE(feature.has_string_id());
    REQUIRE(feature.id() == 8);

    feature = layer.next_feature();
    REQUIRE_FALSE(feature);
}

static vtzero::layer next_nonempty_layer(vtzero::vector_tile& tile) {
    while (auto layer = tile.next_layer()) {
        if (layer && !layer.empty()) {
            return layer;
        }
    }
    return vtzero::layer{};
}

// TODO(joto) this function needs to be updated for vt3
static bool vector_tile_equal(const std::string& t1, const std::string& t2) {
    vtzero::vector_tile vt1{t1};
    vtzero::vector_tile vt2{t2};

    for (auto l1 = next_nonempty_layer(vt1), l2 = next_nonempty_layer(vt2);
         l1 || l2;
         l1 = next_nonempty_layer(vt1), l2 = next_nonempty_layer(vt2)) {

        if (!l1 ||
            !l2 ||
            l1.version() != l2.version() ||
            l1.extent() != l2.extent() ||
            l1.num_features() != l2.num_features() ||
            l1.name() != l2.name()) {
            return false;
        }

        for (auto f1 = l1.next_feature(), f2 = l2.next_feature();
             f1 || f2;
             f1 = l1.next_feature(), f2 = l2.next_feature()) {
            if (!f1 ||
                !f2 ||
                f1.id() != f2.id() ||
                f1.string_id() != f2.string_id() ||
                f1.geometry_type() != f2.geometry_type() ||
                f1.geometry_data() != f2.geometry_data() ||
                f1.elevations_data() != f2.elevations_data() ||
                f1.attributes_data() != f2.attributes_data()) {
                return false;
            }
            for (auto p1 = f1.next_property(), p2 = f2.next_property();
                p1 || p2;
                p1 = f1.next_property(), p2 = f2.next_property()) {
                if (!p1 ||
                    !p2 ||
                    p1.key() != p2.key() ||
                    p1.value() != p2.value()) {
                    return false;
                }
            }
        }
    }

    return true;
}

TEST_CASE("vector_tile_equal") {
    REQUIRE(vector_tile_equal("", ""));

    const auto buffer = load_test_tile();
    REQUIRE(buffer.size() == 269388);
    REQUIRE(vector_tile_equal(buffer, buffer));

    REQUIRE_FALSE(vector_tile_equal(buffer, ""));
}

TEST_CASE("Copy tile") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    while (auto layer = tile.next_layer()) {
        vtzero::layer_builder lbuilder{tbuilder, layer};
        while (auto feature = layer.next_feature()) {
            vtzero::copy_feature(feature, lbuilder);
        }
    }

    const std::string data = tbuilder.serialize();
    REQUIRE(vector_tile_equal(buffer, data));
}

TEST_CASE("Copy tile using geometry_2d_feature_builder") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    while (auto layer = tile.next_layer()) {
        vtzero::layer_builder lbuilder{tbuilder, layer};
        while (auto feature = layer.next_feature()) {
            vtzero::feature_builder<2, false> fbuilder{lbuilder};
            fbuilder.copy_id(feature);
            fbuilder.copy_geometry(feature);
            fbuilder.copy_attributes(feature);
            fbuilder.commit();
        }
    }

    const std::string data = tbuilder.serialize();
    REQUIRE(vector_tile_equal(buffer, data));
}

TEST_CASE("Copy only point geometries using geometry_2d_feature_builder") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    int n = 0;
    while (auto layer = tile.next_layer()) {
        vtzero::layer_builder lbuilder{tbuilder, layer};
        while (auto feature = layer.next_feature()) {
            vtzero::feature_builder<2, false> fbuilder{lbuilder};
            fbuilder.set_integer_id(feature.id());
            if (feature.geometry_type() == vtzero::GeomType::POINT) {
                fbuilder.copy_geometry(feature);
                while (auto property = feature.next_property()) {
                    fbuilder.add_property(property.key(), property.value());
                }
                fbuilder.commit();
                ++n;
            } else {
                fbuilder.rollback();
            }
        }
    }
    REQUIRE(n == 17);

    const std::string data = tbuilder.serialize();

    n = 0;
    vtzero::vector_tile result_tile{data};
    while (auto layer = result_tile.next_layer()) {
        while (auto feature = layer.next_feature()) {
            ++n;
        }
    }

    REQUIRE(n == 17);
}

struct points_to_vector {

    constexpr static const unsigned int max_geometric_attributes = 0;

    std::vector<vtzero::point> m_points{};

    static vtzero::point convert(const vtzero::unscaled_point& p) noexcept {
        return {p.x, p.y};
    }

    void points_begin(const uint32_t count) {
        m_points.reserve(count);
    }

    void points_point(const vtzero::point point) {
        m_points.push_back(point);
    }

    void points_end() const {
    }

    std::vector<vtzero::point> result() {
        return m_points;
    }

}; // struct points_to_vector

TEST_CASE("Copy only point geometries using point_2d_feature_builder") {
    const auto buffer = load_test_tile();
    vtzero::vector_tile tile{buffer};

    vtzero::tile_builder tbuilder;

    int n = 0;
    while (auto layer = tile.next_layer()) {
        vtzero::layer_builder lbuilder{tbuilder, layer};
        while (auto feature = layer.next_feature()) {
            vtzero::point_2d_feature_builder fbuilder{lbuilder};
            fbuilder.copy_id(feature);
            if (feature.geometry_type() == vtzero::GeomType::POINT) {
                const auto points = feature.decode_point_geometry(points_to_vector{});
                fbuilder.add_points_from_container(points);
                fbuilder.copy_attributes(feature);
                fbuilder.commit();
                ++n;
            } else {
                fbuilder.rollback();
            }
        }
    }
    REQUIRE(n == 17);

    const std::string data = tbuilder.serialize();

    n = 0;
    vtzero::vector_tile result_tile{data};
    while (auto layer = result_tile.next_layer()) {
        while (auto feature = layer.next_feature()) {
            ++n;
        }
    }

    REQUIRE(n == 17);
}

TEST_CASE("Build point feature from container with too many points") {

    // fake container pretending to contain too many points
    struct test_container {

        std::size_t size() const noexcept {
            return 1ul << 29u;
        }

        vtzero::point* begin() const noexcept {
            return nullptr;
        }

        vtzero::point* end() const noexcept {
            return nullptr;
        }

    };

    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    fbuilder.set_integer_id(1);

    test_container tc;
    REQUIRE_THROWS_AS(fbuilder.add_points_from_container(tc), const vtzero::geometry_exception&);
}

TEST_CASE("Moving a feature builder is allowed") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    vtzero::point_2d_feature_builder fbuilder{lbuilder};

    auto fbuilder2 = std::move(fbuilder);
    vtzero::point_2d_feature_builder fbuilder3{std::move(fbuilder2)};
}

