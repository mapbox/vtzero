#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero examples
#
#-----------------------------------------------------------------------------

set(TEST_SOURCES test_main.cpp tests.cpp fixture_tests.cpp)

add_executable(vtzero-create vtzero-create.cpp)
add_executable(vtzero-filter vtzero-filter.cpp utils.cpp)
add_executable(vtzero-show vtzero-show.cpp utils.cpp)


#-----------------------------------------------------------------------------
