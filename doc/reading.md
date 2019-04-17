
# Reading vector tiles

To access the contents of vector tiles with vtzero create a `vector_tile`
object first with the data of the tile as first argument:

```cpp
#include <vtzero/vector_tile.hpp> // always needed when reading vector tiles

std::string vt_data = ...;
vtzero::vector_tile tile{vt_data};
```

Instead of a string, you can also initialize the `vector_tile` using a
`vtzero::data_view`. This class contains only a pointer and size referencing
some data similar to the C++17 `std::string_view` class. It is typedef'd from
the `protozero::data_view`. See [the protozero
doc](https://github.com/mapbox/protozero/blob/master/doc/advanced.md#protozero_use_view)
for more details.

```cpp
vtzero::data_view vt_data = ...;
vtzero::vector_tile tile{vt_data};
```

In both cases the `vector_tile` object contains references to the original
tile data. You have to make sure this data stays available through the whole
lifetime of the `vector_tile` object and all the other objects we'll create
in this tutorial for accessing parts of the vector tile. The data is **not**
copied by vtzero when accessing vector tiles.

You can think of the `vector_tile` class as a "proxy" class giving you access
to the decoded data, similarly the `layer` and `feature` classes described in
the next chapters are "proxy" classes, too.

## Accessing layers

Vector tiles consist of a list of layers. The list of layers can be empty
in which case `tile.empty()` will return true.

The simplest and fasted way to access the layers is through an iterator
you get from the tile using the usual `begin()` and `end()` functions.
You can use it in a range-based for like this:

```cpp
for (auto layer : tile) {
    ...
}
```

Note that this creates new layer objects on the fly referencing the layer you
are currently looking at.

You can also access layers through their index or name:

```cpp
tile.get_layer(3);
```

will give you the 4th layer in the tile. With

```cpp
tile.get_layer_by_name("foobar");
```

you'll get the layer with the specified name. Both will return the invalid
layer if that layer doesn't exist.

Note that if you are interested in several layers, accessing them by index or
name is less efficient than iterating over them. So usually you should only use
those function if you want to access one specific layer only.

If you need the number of layers, you can call `tile.count_layers()`. This
function still has to iterate over the layers internally decoding some of the
data, so it is not cheap.

## The layer

Once you have a layer as described in the previous chapter you can access the
metadata of this layer easily:

* The version is available with `layer.version()`. Only version 1 to 3 are
  currently supported by this library.
* The extent of the tile is available through `layer.extent()`. This is usually
  4096.
* Version 3 tiles can optionally containt the tile x/y/zoom. You can access
  this with `layer.get_tile()` which returns a `tile` object (not to be
  confused with a `vector_tile`) containing the x and y coordinates and the
  zoom.
* The function `layer.name()` returns the name of the layer as `data_view`.
  This does **not** include a final 0-byte!
* The number of features is returned by the `layer.num_features()` function.
  If it doesn't contain any features `layer.empty()` will return true.
  (Different then the `vector_tile::count_layers()`, the `layer::num_features()`
  function is `O(1)`).

To access the features use an iterator you get from the layer using the usual
`begin()` and `end()` functions. You can use it in a range-based for like this:

```cpp
for (auto feature : layer) {
    ...
}
```

If you know the integer ID of a feature you can get the feature using
`get_feature_by_id()`, but note that this will do a linear search through
all the features in the layer, decoding each one until it finds the right ID.
This is almost always **not** what you want.

Note that the features returned by these functions will internally contain a
pointer to the layer it came from. So the layer has to stay valid as long as
the feature is used.

## The feature

You get features from the layer as described in the previous chapter. The
`feature` class gives you access to the ID, the geometry and the attributes
of the feature.

Layers can optionally have an integer ID or a string ID (version 3 only).
Access the ID using the `integer_id()` or `string_id()` methods. You can ask
for the existence of the ID using `has_id()`, `has_integer_id()`, and
`has_string_id()`. String IDs are returned as `data_view` objects:

```cpp
const auto feature = *layer.begin();
if (feature.has_integer_id()) {
    cout << feature.integer_id() << '\n';
} else if (feature.has_string_id()) {
    cout << std::string(feature.string_id()) << '\n';
}
```

## Attributes


## The property

Each property you get from the feature is an object of the `property` class. It
contains a view of the property key and value. The key is always a string
encoded in a `vtzero::data_view`, the value can be of different types but is
always encapsulated in a `property_value` type, a variant type that can be
converted into whatever type the value really has.

```cpp
auto property = ...;
auto pkey = std::string(property.key()); // returns a vtzero::data_view which
                                         // can be converted to std::string
property_value pvalue = property.value();
```

To get the type of the property value, call `type()`:

```cpp
const auto type = pvalue.type();
```

If the property value is an int, for instance, you can get it like this:

```cpp
if (pvalue.type() == property_value_type::int_value)
    int64_t v = pvalue.int_value();
}
```

Instead of accessing the values this way, you'll often use the visitor
interface. Here is an example where the `print_visitor` struct is used to print
out the values. In this case one overload is used for all primitive types
(`double`, `float`, `int`, `uint`, `bool`), one overload is used for the `string_value`
type which is encoded in a `data_view`. You must make sure your visitor handles
all those types.

```cpp
struct print_visitor {

    template <typename T>
    void operator()(T value) {
        std::cout << value;
    }

    void operator()(vtzero::data_view value) {
        std::cout << std::string(value);
    }

};

vtzero::apply_visitor(print_visitor{}, pvalue));
```

All call operators of your visitor class have to return the same type. In the
case above this was `void`, but it can be something else. That return type
will be the return type of the `apply_visitor` function. This can be used,
for instance, to convert the values into one type:

```cpp
struct to_string_visitor {

    template <typename T>
    std::string operator()(T value) {
        reutrn std::to_string(value);
    }

    std::string operator()(vtzero::data_view value) {
        return std::string(value);
    }

};

std::string v = vtzero::apply_visitor(to_string_visitor{}, pvalue);
```

Sometimes you want to convert the `property_value` type into your own variant
type. You can use the `vtzero::convert_property_value()` free function for
this.

Lets say you are using `boost` and this is your variant:

```cpp
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
```

You can then use the following line to convert the data:
```cpp
variant_type v = vtzero::convert_property_value<variant_type>(pvalue);
```

Your variant type must be constructible from all the types `std::string`,
`float`, `double`, `int64_t`, `uint64_t`, and `bool`. If it is not, you can
define a mapping between those types and the types you use in your variant
class.

```cpp
using variant_type = boost::variant<mystring, double, int64_t, uint64_t, bool>;

struct mapping : vtzero::property_value_mapping {
    using string_type = mystring; // use your own string type which must be
                                  // convertible from data_view
    using float_type = double; // no float in variant, so convert to double
};

variant_type v = vtzero::convert_property_value<variant_type, mapping>(pvalue);
```

## Creating a properties map

This linear access to the properties with lazy decoding of each property only
when it is accessed saves memory allocations, especially if you are only
interested in very few properties. But sometimes it is easier to create a
mapping (based on `std::unordered_map` for instance) between keys and values. This is where
the `vtzero::create_properties_map()` templated free function comes in. It
needs the map type as template parameter:

```cpp
using key_type = std::string; // must be something that can be converted from data_view
using value_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
using map_type = std::map<key_type, value_type>;

auto feature = ...;
auto map = create_properties_map<map_type>(feature);
```

Both `std::map` and `std::unordered_map` are supported as map type, but this
should also work with any other map type that has an `emplace()` method.

## Geometries

Features must contain a geometry of type UNKNOWN, POINT, LINESTRING, POLYGON,
or SPLINE. The UNKNOWN type is not further specified by the vector tile spec,
this library doesn't allow you to do anything with this type. Note that
multipoint, multilinestring, multipolygon, and multispline geometries are also
possible, they don't have special types. You can get the geometry type with
`feature.geometry_type()`.

To decode the data you have to call one of the decoder functions on the
`feature` object: `decode_geometry()`, `decode_point_geometry()`,
`decode_linestring_geometry()`, `decode_polygon_geometry()`, or
`decode_spline_geometry()`. The first of these functions can decode any type of
geometry. The others must be called with a geometry of the specified type and
will only decode that type.

For all the decoder functions the only parameter is a *handler* object that you
must implement. The decoder function will call certain callbacks on this object
that give you part of the geometry data which allows you to use this data in
any way you like.

The handler for `decode_point_geometry()` must implement the following
functions:

* `void points_begin(uint32_t count)`: This is called once at the beginning
  with the number of points. For a point geometry, this will be 1, for
  multipoint geometries this will be larger.
* `void points_point(vtzero::point point)`: This is called once for each
  point.
* `void points_end()`: This is called once at the end.

The handler for `decode_linestring_geometry()` must implement the following
functions:

* `void linestring_begin(uint32_t count)`: This is called at the beginning
  of each linestring with the number of points in this linestring. For a simple
  linestring this function will only be called once, for a multilinestring
  it will be called several times.
* `void linestring_point(vtzero::point point)`: This is called once for each
  point.
* `void linestring_end()`: This is called at the end of each linestring.

The handler for `decode_polygon_geometry` must implement the following
functions:

* `void ring_begin(uint32_t count)`: This is called at the beginning
  of each ring with the number of points in this ring. For a simple polygon
  with only one outer ring, this function will only be called once, if there
  are inner rings or if this is a multipolygon, it will be called several
  times.
* `void ring_point(vtzero::point point)`: This is called once for each
  point.
* `void ring_end(vtzero::ring_type)`: This is called at the end of each ring.
  The parameter tells you whether the ring is an outer or inner ring or whether
  the ring was invalid (if the area is 0).

The handler for `decode_geometry()` must implement all of the functions
mentioned above for the different types. It is guaranteed that only one
set of functions will be called depending on the geometry type.

If your handler implements the `result()` method, the decode functions will
have the return type of the `result()` method and will return whatever
result returns. If the `result()` method is not available, the decode functions
return void.

Here is a typical implementation of a linestring handler:

```cpp
struct linestring_handler {

    using linestring = std::vector<my_point_type>;

    linestring points;

    void linestring_begin(uint32_t count) {
        points.reserve(count);
    }

    void linestring_point(vtzero::point point) noexcept {
        points.push_back(convert_to_my_point(point));
    }

    void linestring_end() const noexcept {
    }

    linestring result() {
        return std::move(points);
    }

};
```

Note that the `count` given to the `linestring_begin()` method is used here to
reserve memory. This is potentially problematic if the count is large. Please
keep this in mind.

## Accessing the key/value lookup tables in a layer

Vector tile layers can contain several tables with attribute keys and values
used in the features in that layer. Vtzero can handle lookups in those tables
internally without you noticing or you can do it yourself.

From the layer object you can get references to the tables. Depending on
the version of the layer only some of these tables are available:

```cpp
vtzero::layer layer = ...;
const auto& kt = layer.key_table(); // version 1/2 and 3
const auto& vt = layer.value_table(); // version 1/2 attributes
const auto& st = layer.string_table(); // version 3 string attributes
const auto& dt = layer.double_table(); // version 3 double attributes
const auto& ft = layer.float_table(); // version 3 float attributes
const auto& it = layer.int_table(); // version 3 int/uint attributes
```

You can also lookup keys and values directly using methods on the layer object:
```cpp
vtzero::layer layer = ...;
const vtzero::data_view k = layer.key(17);
const vtzero::property_value_view v = layer.value(42);
```

As usual in vtzero you only get views back, so you need to keep the layer
object around as long as you are accessing the results of those methods.

Note that the lookup tables are created on first access from the layer data. As
long as you are not accessing those tables directly or by looking up any
properties in a feature, the tables are not created and no extra memory is
used.

