
# Vtzero Tutorial

The vtzero header-only library is used to read and write vector tile data
as specified in the [Mapbox Vector Tile
Specification](https://github.com/mapbox/vector-tile-spec). This document
assumes that you are familiar with that specification.

## Overview

The library has basically two parts: The part concerned with decoding existing
vector tiles and the part concerned with creating new vector tiles. You can
use either one without knowing much about the other side, but it is, of course
also possible to read parts of a vector tile and stick it into a new one.

Vtzero is trying to do as little work as possible while still giving you a
reasonably easy to use interface. This means that it will, as much as feasible,
decode the different parts of a vector tile only when you ask for them. Most
importantly it will try to avoid memory allocation and it will not copy data
around unnecessarily but work with references instead.

On the writing side it means that you have to call the API in a specific order
when adding more data to a vector tile. This allows vtzero to avoid multiple
copies of the data.

## Basic types

Vtzero contains several basic small (value) types such as `GeomType`,
`property_value_type`, `index_value`, `index_value_pair`, and `point` which
hold basic values in a type-safe way. Most of them are defined in `types.hpp`
(`point` is in `geometry.hpp`).

Sometimes it is useful to be able to print the values of those types, for
instance when debugging. For this overloads of `operator<<` on `basic_ostream`
are available in `output.hpp`. Include this file and you can use the usual
`std::cout << some_value;` to print those values.

## Reading vector tiles

To access the contents of vector tiles with vtzero create a `vector_tile`
object first with the data of the tile as first argument:

```cpp
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
to the decoded data, similarly the classes `layer`, `feature`, and
`property` described in the next chapters are "proxy" classes, too.

### Accessing layers

Vector tiles consist of a list of layers. The list of layers can be empty
in which case `tile.empty()` will return true.

The simplest and fasted way to access the layers is through the `next_layer()`
function:

```cpp
while (auto layer = tile.next_layer()) {
    ...
}
```

Note that this creates new layer objects on the fly referencing the layer you
are currently looking at. Once you have iterated over all the layers,
`next_layer()` will return the "invalid" (default constructed) layer object
which converts to false in an boolean context.

You can reset the layer iterator to the beginning again if you need to go
over the layers again:

```cpp
tile.reset_layer();
```

Instead of using this external interator, you can use a different function with
an internal iterator that calls a function defined by you for each layer. Your
function must take a `layer&&` as parameter and return `true` if the iteration
should continue and `false` otherwise:

```cpp
tile.for_each_layer([&](layer&& l) {
    // do something with layer
    return true;
});
```

Both the external and internal iteration do basically the same and have the
same performance characteristics.

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

Note that accessing layers by index or name is less efficient than iterating
over them using `next_layer()` if you are accessing several layers. So usually
you should only use those function if you want to access one specific layer
only.

If you need the number of layers, you can call `tile.count_layers()`. This
function still has to iterate over the layers internally decoding some of the
data, so it is not cheap.

### The layer

Once you have a layer as described in the previous chapter you can access the
metadata of this layer easily:

* The version is available with `layer.version()`. Only version 1 and 2 are
  currently supported by this library.
* The extent of the tile is available through `layer.extent()`. This is usually
  4096.
* The function `layer.name()` returns the name of the layer as `data_view`.
  This does **not** include a final 0-byte!
* The number of features is returned by the `layer.num_features()` function.
  If it doesn't contain any features `layer.empty()` will return true.
  (Different then the `vector_tile::count_layers()`, the `layer::num_features()`
  function is `O(1)`).

To access the features call the `next_feature()` function until it returns
the invalid (default constructed) feature:

```cpp
while (auto feature = layer.next_feature()) {
    ...
}
```

Use `reset_feature()` to restart the feature iterator from the beginning.

Instead of using this external interator, you can use a different function with
an internal iterator that calls a function defined by you for each feature.
Your function must take a `feature&&` as parameter and return `true` if the
iteration should continue and `false` otherwise:

```cpp
layer.for_each_feature([&](feature&& f) {
    // do something with the feature
    return true;
});
```

Both the external and internal iteration do basically the same and have the
same performance characteristics.

If you know the ID of a feature you can get the feature using
`get_feature_by_id()`, but note that this will do a linear search through
all the features in the layer, decoding each one until it finds the right ID.
This is almost always **not** what you want.

### The feature

You get features from the layer as described in the previous chapter. The
`feature` class gives you access to the ID, the geometry and the properties
of the feature. Access the ID using the `id()` method which will return 0
if no ID is set. You can ask for the existence of the ID using `has_id()`:

```cpp
auto feature = layer...;
if (feature.has_id()) {
    cout << feature.id() << '\n';
}
```

The `geometry()` method returns an object of the `geometry` class. It contains
the geometry type and a reference to the (undecoded) geometry data. See a
later chapter on the details of decoding this geometry. You can also directly
add this geometry to a new feature you are writing.

The number of properties in the feature is returned by the
`feature::num_properties()` function. If the feature doesn't contain any
properties `feature.empty()` will return true. (Different then the
`vector_tile::count_layers()`, the `feature::num_properties()` function is
`O(1)`).

To access the properties call the `next_property()` function until it returns
the invalid (default constructed) property:

```cpp
while (auto property = feature.next_property()) {
    ...
}
```

Use `reset_property()` to restart the property iterator from the beginning.

Instead of using this external interator, you can use a different function with
an internal iterator that calls a function defined by you for each property.
Your function must take a `property&&` as parameter and return `true` if the
iteration should continue and `false` otherwise:

```cpp
feature.for_each_property([&](property&& p) {
    ...
    return true;
});
```

Both the external and internal iteration do basically the same and have the
same performance characteristics.

### The property

Each property you get from the feature is an object of the `property` class. It
contains a view of the property key and value. The key is always a string
encoded in a `vtzero::data_view`, the value can be of different types but is
always encapsulated in a `property_value` type, a variant type that can be
converted into whatever type the value really has.

```cpp
auto property = ...;
std::string pkey = property.key(); // returns a vtzero::data_view which can
                                   // be converted to std::string
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
(double, float, int, uint, bool), one overload is used for the `string_value`
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

### Creating a properties map

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

### Geometries

Features must contain a geometry of type UNKNOWN, POINT, LINESTRING, or
POLYGON. The UNKNOWN type is not further specified by the vector tile spec,
this library doesn't allow you to do anything with this type. Note that
multipoint, multilinestring, and multipolygon geometries are also possible,
they don't have special types.

You can get the geometry type with `feature.geometry_type()`, but usually
you'll get the geometry with `feature.geometry()`. This will return an object
of type `vtzero::geometry` which contains the geometry type and a view of
the raw geometry data. To decode the data you have to call one of the decoder
free functions `decode_geometry()`, `decode_point_geometry()`,
`decode_linestring_geometry()`, or `decode_polygon_geometry()`. The first of
these functions can decode any point, linestring, or polygon geometry. The
others must be called with a geometry of the specified type and will only
decode that type.

For all the decoder functions the first parameter is the geometry (as returned
by `feature.geometry()`), the second parameter is a *handler* object that you
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

### Accessing the key/value lookup tables in a layer

Vector tile layers contain two tables with all the property keys and all
property values used in the features in that layer. Vtzero usually handles
those table lookups internally without you noticing. But sometimes it might
be necessary to access this data directly.

From the layer object you can get references to the tables:

```cpp
vtzero::layer layer = ...;
const auto& kt = layer.key_table();
const auto& vt = layer.value_table();
```

Instead you can also lookup keys and values using methods on the layer object:
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

## Writing vector tiles

Writing vector tiles start with creating a `tile_builder`. This builder will
then be used to add layers and features in those layers. Once all this is done,
you call `serialize()` to actually build the vector tile from the data you
provided to the builders:

```cpp
vtzero::tile_builder tbuilder;
// add lots of data to builder...
std::string buffer = tbuilder.serialize();
```

You can also serialize the data into an existing buffer instead:

```cpp
std::string buffer; // got buffer from somewhere
tbuilder.serialize(buffer);
```

### Adding layers

Once you have a tile builder, you'll first need some layers:

```cpp
vtzero::tile_builder tbuilder;
vtzero::layer_builder layer_pois{tbuilder, "pois", 2, 4096};
vtzero::layer_builder layer_roads{tbuilder, "roads"};
vtzero::layer_builder layer_forests{tbuilder, "forests"};
```

Here three layers called "pois", "roads", and "forests" are added. The first
one explicitly specifies the vector tile version used and the extent. The
values specified here are the default, so all layers in this example will have
a version of 2 and an extent of 4096.

If you have read a layer from an existing vector tile and want to copy over
some of the data, you can use this layer to initialize the new layer in the
new vector tile with the name, version and extent from the existing layer like
this:

```cpp
vtzero::layer some_layer = ...;
vtzero::layer_builder layer_pois{tbuilder, some_layer};
// same as...
vtzero::layer_builder layer_pois{tbuilder, some_layer.name(),
                                           some_layer.version(),
                                           some_layer.extent()};
```

If you want to copy over an existing layer completely, you can use the
`add_existing_layer()` function instead:

```cpp
vtzero::layer some_layer = ...;
vtzero::tile_builder tbuilder;
tbuilder.add_existing_layer(some_layer);
```

Or, if you have the encoded layer data available in a `data_view` this also
works:

```cpp
vtzero::data_view layer_data = ...;
vtzero::tile_builder tbuilder;
tbuilder.add_existing_layer(layer_data);
```

Note that this call will only store a reference to the data to be added in the
tile builder. The data will only be copied when the final `serialize()` is
called, so the input data must still be available then!

You can mix any of the ways of adding a layer to the tile mentioned above. The
layers will be added to the tile in the order you add them to the
`tile_builder`.

The tile builder is smart enough to not add empty layers, so you can start
out with all the layers you might need and if some of them stay empty, they
will not be added to the tile when `serialize()` is called.

## Adding features

Once we have one or more `layer_builder`s instantiated, we can add features
to them. This is done through the following feature builder classes:

* `point_feature_builder` to add a feature with a (multi)point geometry,
* `linestring_feature_builder` to add a feature with a (multi)linestring
  geometry,
* `polygon_feature_builder` to add a feature with a (multi)polygon geometry, or
* `geometry_feature_builder` to add a feature with an existing geometry you
  got from reading a vector tile.

In all cases you need to instantiate the feature builder class, optionally
add the feature ID using the `set_id()` method, add the geometry and then
add all the properties of this feature. You have to keep to this order!

```cpp
...
vtzero::layer_builder lbuilder{...};
{
    vtzero::point_feature_builder fbuilder{lbuilder};
    // optionally set the ID
    fbuilder.set_id(23);
    // add the geometry (exact calls are different for different feature builders)
    fbuilder.add_point(99, 33);
    // add the properties
    fbuilder.add_property("highway", "primary");
    fbuilder.add_property("maxspeed", 80);
}
```

Once the feature builder is destructed, the feature is "committed" to the
layer. Instead you can also call `commit()` on the feature builder to do this.
(After calling `commit()` you can't change the feature builder object any
more.)

If you decide that you do not want to add this feature to the layer after all,
you can call `rollback()` on the feature builder. This can happen for instance
if you detect that you have an invalid geometry while you are adding the
geometry to the feature builder.

There are different ways of adding the geometry to the feature, depending on
the geometry type.

### Adding a point geometry

Simply call `add_point()` to set the point geometry. There are three different
overloads for this function. One takes a `vtzero::point`, one takes two
`uint32_t`s with the x and y coordinates and one takes any type `T` that can
be converted to a `vtzero::point` using the `create_vtzero_point` function.
This templated function works on any type that has `x` and `y` members and
you can create your own overload of this function. See the
[advanced.md](advanced topics documentation).

### Adding a multipoint geometry

Call `add_points()` with the number of points in the geometry as only argument.
After that call `set_point()` for each of those points. `set_point()` has
multiple overloads just like the `add_point()` method described above.

There are two other versions of the `add_points()` function. They take two
iterators defining a range to get the points from. Dereferencing those
iterators must yield a `vtzero::point` or something convertible to it. One
of these functions takes a third argument, the number of points the iterator
will yield. If this is not available `std::distance(begin, end)` is called
which internally by the `add_points()` function which might be slow depending
on your iterator type.

There is also a `add_points_from_container()` function which copies the
point from any container type supporting the `size()` function and which
iterator yields a `vtzero::point` or something convertible to it.

### Adding a linestring geometry

Call `add_linestring()` with the number of points in the linestring as only
argument. After that call `set_point()` for each of those points. `set_point()`
has multiple overloads just like the `add_point()` method described above.

There are two other versions of the `add_linestring()` function. They take two
iterators defining a range to get the points from. Dereferencing those
iterators must yield a `vtzero::point` or something convertible to it. One of
these functions takes a third argument, the number of points the iterator will
yield. If this is not available `std::distance(begin, end)` is called which
internally by the `add_linestring()` function which might be slow depending on
your iterator type.

### Adding a multilinestring geometry

Adding a multilinestring works just like adding a linestring, just do the
calls to `add_linestring()` etc. repeatedly for each of the linestrings.

### Adding a polygon geometry

A polygon consists of one outer ring and zero or more inner rings. You have
to first add the outer ring and then the inner rings, if any.

Call `add_ring()` with the number of points in the ring as only argument. After
that call `set_point()` for each of those points. `set_point()` has multiple
overloads just like the `add_point()` method described above.

There are two other versions of the `add_ring()` function. They take two
iterators defining a range to get the points from. Dereferencing those
iterators must yield a `vtzero::point` or something convertible to it. One of
these functions takes a third argument, the number of points the iterator will
yield. If this is not available `std::distance(begin, end)` is called which
internally by the `add_ring()` function which might be slow depending on your
iterator type.

### Adding a multipolygon geometry

Adding a multipolygon works just like adding a polygon, just do the calls to
`add_ring()` etc. repeatedly for each of the rings. Make sure to always first
add an outer ring, then the inner rings in this outer ring, then the next
outer ring and so on.

### Adding an existing geometry

The `geometry_feature_builder` class is used to add geometries you got from
reading a vector tile. This is useful when you want to copy over a geometry
from a feature without decoding it.

```cpp
auto geom = ... // get geometry from a feature you are reading
...
vtzero::tile_builder tb;
vtzero::layer_builder lb{tb};
vtzero::geometry_feature_builder fb{lb};
fb.set_id(123); // optionally set ID
fb.add_geometry(geom) // add geometry
fb.add_property("foo", "bar"); // add properties
...
```

## Adding properties

A feature can have any number of properties. They are added with the
`add_property()` method called on the feature builder. There are two different
ways of doing this. The *simple approach* which does all the work for you and
the *advanced approach* which can be more performant, but you have to to some
more work. It is recommended that you start out with the simple approach and
only switch to the advanced approach once you have a working program and want
to get the last bit of performance out of it.

The difference stems from the way properties are encoded in vector tiles. While
properties "belong" to features, they are really stored in two tables (for the
keys and values) in the layer. The individual feature only references the
entries in those tables by index. This make the encoded tile smaller, but it
means somebody has to manage those tables. In the simple approach this is done
behind the scenes by the `layer_builder` object, in the advanced approach you
handle that yourself.

Do not mix the simple and the advanced approach unless you know what you are
doing.

### The simple approach to adding properties

For the simple approach call `add_property()` with two arguments. The first is
the key, it must have some kind of string type (`std::string`, `const char*`,
`vtzero::data_view`, anything really that converts to a `data_view`). The
second argument is the value, for which most basic C++ types are allowed
(string types, integer types, double, ...). See the API documentation for the
constructors of the `encoded_property_value` class for a list.

```cpp
vtzero::layer_builder lb{...};
vtzero::linestring_feature_builder fb{lb};
...
fb.add_property("waterway", "stream"); // string value
fb.add_property("name", "Little Creek");
fb.add_property("width", 1.5); // double value
...
```

Sometimes you need to specify exactly which type should be used in the
encoding. The `encoded_property_value` constructor can take special types for
that like in the following example, where you force the `sint` encoding:

```cpp
fb.add_property("layer", vtzero::sint_value_type(2));
```

You can also call `add_property()` with a single `vtzero::property` argument
(which is handy if you are copying this property over from a tile you are
reading):

```cpp
while (auto property = feature.next_property()) {
    if (property.key() == "name") {
        feature_builder.add_property(property);
    }
}
```

### The advanced approach to adding properties

In the advanced approach you have to do the indexing yourself. Here is a very
basic example:

```cpp
vtzero::tile_builder tbuilder;
vtzero::layer_builder lbuilder{tbuilder, "test"};
const vtzero::index_value highway = lbuilder.add_key("highway");
const vtzero::index_value primary = lbuilder.add_value("primary");
...
vtzero::point_feature_builder fbuilder{lbuilder};
...
fbuilder.add_property(highway, primary);
...
```

The methods `add_key()` and `add_value()` on the layer builder are used to add
keys and values to the tables in the layer. They both return the index (of type
`vtzero::index_value`) of those keys or values in the tables. You store
those index values somewhere (in this case in the `highway` and `primary`
variables) and use them when calling `add_property()` on the feature builder.

In some cases you only have a few property keys and know them beforehand,
then storing the key indexes in individual variables might work. But for
values this usually doesn't make much sense, and if all your keys and values
are only known at runtime, it doesn't work either. For this you need some kind
of index datastructure mapping from keys/values to index values. You can
implement this yourself, but it is easier to use some classes provided by
vtzero. Then the code looks like this:

```cpp
#include <vtzero/index.hpp> // use this include to get the index classes
...
vtzero::layer_builder lb{...};
vtzero::key_index<std::map> key_index{lb};
vtzero::value_index_internal<std::unordered_map> value_index{lb};
...
vtzero::point_feature_builder fb{lb};
...
fb.add_property(key_index("highway"), value_index("primary"));
...
```

In this example the `key_index` template class is used for keys, it uses
`std::map` internally as can be seen by its template argument. The
`value_index_internal` template class is used for values, it uses
`std::unordered_map` internally in this example. Whether you specify `std::map`
or `std::unordered_map` or something else (that needs to be compatible to those
classes) is up to you. Benchmark your use case and decide then.

Keys are always strings, so they are easy to handle. For keys there is only the
single `key_index` in vtzero.

For values this is more difficult. Basically there are two choices:

1. Encode the value according to the vector tile encoding rules which results
   in a string and store this in the index. This is what the
   `value_index_internal` class does.
2. Store the unencoded value in the index. The index lookup will be faster,
   but you need a different index type for each value type. This is what the
   `value_index` classes do.

The `value_index` template classes need three template arguments: The type
used internally to encode the value, the type used externally, and the map
type.

In this example the user program has the values as `int`, the index will store
them in a `std::map<int>`. The integer value is then encoded in an `sint`
int the vector tile:

```cpp
vtzero::value_index<vtzero::sint_value:type, int, std::map> index;
```

Sometimes these generic indexes based on `std::map` or `std::unordered_map`
are inefficient, that's why there are specialized indexes for special cases:

* The `value_index_bool` class can only index bool values.
* The `value_index_small_uint` class can only index small unsigned integer
  values (up to `uint16_t`). It uses a vector internally, so if all your
  numbers are small and densly packed, this is very efficient. This is
  especially useful for enum types.


## Error handling

Many vtzero functions can throw exceptions. Most of them fall into two
categories:

* If the underlying protocol buffers data has some kind of problem, you'll
  get an exception from the [protozero
  library](https://github.com/mapbox/protozero/blob/master/doc/tutorial.md#asserts-and-exceptions-in-the-protozero-library).
  They are all derived from `protozero::exception`.
* If the protocol buffers data is okay, but the vector tile data is invalid
  in some way, you'll get an exception from vtzero library.

All the exceptions in the vtzero library are derived from `vtzero::exception`.
The exceptions are:

* A `format_exception` is thrown when vector tile encoding isn't valid
  according to the vector tile specification.
* A `geometry_exception` is thrown when a geometry encoding isn't valid
  according to the vector tile specification.
* A `type_exception` is thrown when a property value is accessed using the
  wrong type.
* A `version_exception` is thrown when an unknown version number is found in
  the layer. Currently vtzero only supports version 1 and 2.
* An `out_of_range_exception` is thrown when an index into the key or value
  table in a layer is out of range. This can only happen if the tile data is
  invalid.

