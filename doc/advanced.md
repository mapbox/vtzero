
# Advanced vtzero topics

## Differences between the specification and the vtzero implementation

The [protobuf specification
says](https://developers.google.com/protocol-buffers/docs/encoding#optional)
that a decoder library must handle repeated *non-packed* fields if repeated
*packed* fields are expected and it must handle multiple repeated packed fields
as if the items are concatenated. Encoders should never encode fields in this
way, though, so it is very unlikely that this would ever happen. For
performance reasons vtzero doesn't handle this case.

The vtzero spec clearly says that you can not have two layers with the same
name in a vector tile. For performance reasons this is neither checked on
reading nor on writing.

