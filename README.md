# vtzero

Minimalistic vector tile decoder and encoder in C++.

Implements the [Mapbox Vector Tile Specification 2.x](https://www.mapbox.com/vector-tiles/specification).

[![Build Status](https://travis-ci.org/mapbox/vtzero.svg?branch=master)](https://travis-ci.org/mapbox/vtzero)
[![Coverage Status](https:/codecov.io/gh/mapbox/vtzero/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/vtzero)

## Status

**This is experimental code and subject to change.**


## Depends

* C++11 compiler
* make
* [Protozero](https://github.com/mapbox/protozero)


## Build

Call `make` to build the tests and examples. Call `make test` to build and
run the tests.

You might have to set `PROTOZERO_INCLUDE` in the Makefile.


## Examples

Several examples are provided to show usage of the library.

Call

    examples/vtzero-create

to create test tile named `test.mvt`.

Call

    examples/vtzero-show TILE-FILE

to show contents of `TILE-FILE`.


## AUTHOR

Jochen Topf (jochen@topf.org)

