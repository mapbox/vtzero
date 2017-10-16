
# vtzero Tutorial

The vtzero header-only library is used to read and write vector tile data
as specified in the [Mapbox Vector Tile
Specification](https://github.com/mapbox/vector-tile-spec). This document
assumes that you are familiar with that specification.

## Overview




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

Note that vtzero is trying to do as little work as possible while still giving
you an easy to use interface. This means that it will, as much as feasible,
decode the different parts of a vector tile only when you ask for them.

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

If you know the ID of a feature you can get the feature using
`get_feature_by_id()`, but note that this will do a linear search through
all the features in the layer, decoding each one until it finds the right ID.
This is almost always **not** what you want.



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


