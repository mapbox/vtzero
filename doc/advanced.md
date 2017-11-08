
# Advanced vtzero topics

## Differences between the protocol buffer specification and the vtzero implementation

The [protobuf specification
says](https://developers.google.com/protocol-buffers/docs/encoding#optional)
that a decoder library must handle repeated *non-packed* fields if repeated
*packed* fields are expected and it must handle multiple repeated packed fields
as if the items are concatenated. Encoders should never encode fields in this
way, though, so it is very unlikely that this would ever happen. For
performance reasons vtzero doesn't handle this case.

## Differences between the vector tile specification and the vtzero implementation

The [vector tile specification](https://github.com/mapbox/vector-tile-spec/blob/master/2.1/README.md#41-layers)
clearly says that you can not have two layers with the same
name in a vector tile. For performance reasons this is neither checked on
reading nor on writing.

## The `create_vtzero_point` customization point

The vtzero builder classes have several functions which take a `vtzero::point`
as argument. But chances are that you are using a different point type in your
code. That's why these functions have overloads taking any type `TPoint` that
can be converted to a `vtzero::point`. This conversion is done by calling the
function `create_vtzero_point()`. Vtzero supplies a version of this function
which will work with any type with members `x` and `y`:

```cpp
template <typename TPoint>
vtzero::point create_vtzero_point(TPoint p) noexcept {
    return {p.x, p.y};
}
```

You can define your own overload of that function taking your own point type
as parameter and returning a `vtzero::point`. Vtzero will find your function
using [ADL](http://en.cppreference.com/w/cpp/language/adl) which magically
makes the vtzero builders work with your point type.

## Protection against huge memory use

When decoding a vector tile we got from an unknown source, we don't know what
surprises it might contain. Building data structures based on the vector tile
sometimes means we have to allocate memory and in the worst case this might be
quite a lot of memory. Vtzero usually doesn't allocate any memory when decoding
a tile, except when reading properties, when there is space for lookup tables
allocated. The memory use for these lookup tables is `sizeof(data_view)` times
the number of entries in the key/value table. In the worst case, when a vector
tile basically only contains such a table, memory use is proportional to the
size of the vector tile. But memory use can be an order of magnitude larger
than the tile size! If you are concerned about memory use, limit the size
of the vector tiles you give to vtzero.

When reading geometries from vector tiles, vtzero doesn't need much memory
itself, but the users of vtzero might. In a typical case you might reserve
enough memory to store, say, a linestring, and then fill that memory. To allow
you to do this, vtzero tells you about the number of points in the linestring.
This number comes from the tile and it might be rather large. Vtzero does a
consistency check comparing the number of points the geometry says it has with
the number of bytes used for the geometry and it will throw an exception if the
numbers can't fit. So you are protected against tiny tiles pretending to
contain a huge geometry. But there still could be a medium-sized tile which
gets "blown up" into a huge memory hog. Your representation of a linestring
can be an order of magnitude larger than the minimum 2 bytes per point
needed in the encoded tile.

So again: If you are concerned about memory use, limit the size of the vector
tiles you give to vtzero.

