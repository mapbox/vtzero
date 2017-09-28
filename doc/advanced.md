
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

