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

set(TEST_SOURCES builder
                 exceptions
                 feature
                 geometry
                 layer
                 property_map
                 property_value
                 types
                 vector_tile)

string(REGEX REPLACE "([^;]+)" "t/test_\\1.cpp" _test_sources "${TEST_SOURCES}")

add_executable(unit-tests test_main.cpp ${_test_sources})

add_executable(fixture-tests test_main.cpp fixture_tests.cpp)

add_test(NAME unit-tests
         COMMAND unit-tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(_fixtures ${CMAKE_CURRENT_SOURCE_DIR}/mvt-fixtures/fixtures)
if(EXISTS ${_fixtures})
    message(STATUS "Found test fixtures. Enabled mvt fixture tests.")
    add_test(NAME fixture-tests
             COMMAND fixture-tests)

    set_tests_properties(fixture-tests PROPERTIES
                         ENVIRONMENT "FIXTURES_DIR=${_fixtures}")
else()
    message(WARNING "Disabled mvt fixture tests, because fixtures not found.\n  Install them by calling 'git submodule update --init'.")
endif()


#-----------------------------------------------------------------------------
