#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero tests
#
#-----------------------------------------------------------------------------

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(TEST_SOURCES test_main.cpp tests.cpp fixture_tests.cpp)

add_executable(unit-tests ${TEST_SOURCES})

add_test(NAME unit-tests
         COMMAND unit-tests)

set_tests_properties(unit-tests PROPERTIES
    ENVIRONMENT "FIXTURES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/mvt-fixtures/fixtures")


#-----------------------------------------------------------------------------
