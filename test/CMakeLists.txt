#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero tests
#
#-----------------------------------------------------------------------------

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(unit-tests test_main.cpp tests.cpp)
add_executable(fixture-tests test_main.cpp fixture_tests.cpp)

add_test(NAME unit-tests COMMAND unit-tests)

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
