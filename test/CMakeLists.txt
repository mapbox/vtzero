#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero tests
#
#-----------------------------------------------------------------------------

if(Boost_FOUND)
    message(STATUS "Boost library found: enable testing with boost::variant")
    add_definitions(-DVTZERO_TEST_WITH_VARIANT)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
else()
    message(STATUS "Boost library not found: disable testing with boost::variant")
endif()

include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/catch")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(TEST_SOURCES attributes
                 attributes_variant
                 builder
                 builder_linestring
                 builder_point
                 builder_polygon
                 builder_spline
                 exceptions
                 extended_geometry
                 extended_geometry_point
                 feature
                 geometric_attributes
                 geometry
                 geometry_linestring
                 geometry_point
                 geometry_polygon
                 geometry_spline
                 index
                 layer
                 output
                 point
                 property_value
                 scaling
                 tile
                 types
                 vector_tile)

string(REGEX REPLACE "([^;]+)" "t/test_\\1.cpp" _test_sources "${TEST_SOURCES}")

add_executable(generate-formatting-tests generate-formatting-tests.cpp)

add_executable(formatting-tests test_main.cpp formatting_tests.cpp)

add_executable(unit-tests test_main.cpp ${_test_sources})

add_executable(fixture-tests test_main.cpp fixture_tests.cpp)

add_executable(base-tests test_main.cpp base_tests.cpp)

add_test(NAME formatting-tests
         COMMAND formatting-tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_tests_properties(formatting-tests PROPERTIES
                     ENVIRONMENT "BASE_DIR=data/formatting-tests")

add_test(NAME unit-tests
         COMMAND unit-tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME base-tests
         COMMAND base-tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_tests_properties(base-tests PROPERTIES
                     ENVIRONMENT "BASE_DIR=data/vector-tile-base")

set(_fixtures ${MVT_FIXTURES}/fixtures)
if(EXISTS ${_fixtures})
    message(STATUS "Found test fixtures. Enabled mvt fixture tests.")
    add_test(NAME fixture-tests
             COMMAND fixture-tests)

    set_tests_properties(fixture-tests PROPERTIES
                         ENVIRONMENT "FIXTURES_DIR=${_fixtures}")

    if(NOT WIN32)
        set(_real_world_dir ${MVT_FIXTURES}/real-world)
        file(GLOB real_world ${_real_world_dir}/bangkok/*
                             ${_real_world_dir}/chicago/*
                             ${_real_world_dir}/nepal/*
                             ${_real_world_dir}/norway/*
                             ${_real_world_dir}/sanfrancisco/*
                             ${_real_world_dir}/uruguay/*)

        if(NOT "${real_world}" STREQUAL "")
            execute_process(COMMAND cat ${real_world}
                            OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/real-world-all.mvt)

            add_test(NAME vtzero-show-real-world
                    COMMAND vtzero-show ${CMAKE_BINARY_DIR}/test/real-world-all.mvt)
        endif()
    endif()
else()
    message(WARNING "Disabled mvt fixture tests, because fixtures not found.\n  Install them by calling 'git submodule update --init' in ${CMAKE_SOURCE_DIR}.")
endif()



#-----------------------------------------------------------------------------
