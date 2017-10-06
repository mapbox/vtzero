# vtzero

Minimalistic vector tile decoder and encoder in C++.

Implements the [Mapbox Vector Tile Specification 2.x](https://www.mapbox.com/vector-tiles/specification).

[![Build Status](https://travis-ci.org/mapbox/vtzero.svg?branch=master)](https://travis-ci.org/mapbox/vtzero)
[![Coverage Status](https://codecov.io/gh/mapbox/vtzero/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/vtzero)

## Status

**This is experimental code and subject to change.**


## Depends

* C++11 compiler (GCC 4.8 or higher, clang 3.5 or higher, ...)
* CMake
* Latest [Protozero](https://github.com/mapbox/protozero)


## Build

First clone `protozero`:

```
git clone git@github.com:mapbox/protozero.git
```

Then clone `vtzero` beside `protozero`. By default the `vtzero` build system looks for `protozero` at `../protozero`. If you would like to use `protozero` from a different path you can set `PROTOZERO_INCLUDE`.

Then, inside the `vtzero` directory do:

```
git submodule update --init
```

Finally, to build the examples do:

```
mkdir build
cd build
cmake ..
make
```

Call `ctest` to run the tests.


## Examples

Several examples are provided to show usage of the library.

Call

    examples/vtzero-create

to create test tile named `test.mvt`.

Call

    examples/vtzero-show TILE-FILE

to show contents of `TILE-FILE`.


## Docs

To build the API docs call `make doc` after CMake. The results will be in your
build directory under `doc/html`.


## Authors

Jochen Topf (jochen@topf.org)
Dane Springmeyer (dane@mapbox.com)

