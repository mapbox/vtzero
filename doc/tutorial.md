
# vtzero Tutorial

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

Your variant type must be constructible from all the types `float`, `double`,
`int64_t`, `uint64_t`, and `bool`. It must also be either constructible from
a `std::string`, or you must specify a type that's constructible from
`data_view` as second template parameter:

```cpp
using variant_type = boost::variant<mystring, float, double, int64_t, uint64_t, bool>;
variant_type v = vtzero::convert_property_value<variant_type, mystring>(pvalue);
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


### Error handling

Many vtzero functions can throw exceptions. Most of them fall into two
categories:

* If the underlying protocol buffers data has some kind of problem, you'll
  get an exception from the [protozero
  library](https://github.com/mapbox/protozero/blob/master/doc/tutorial.md#asserts-and-exceptions-in-the-protozero-library).
* If the protocol buffers data is okay, but the vector tile data is invalid
  in some way, you'll get an exception from vtzero library.


