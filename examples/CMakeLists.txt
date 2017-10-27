#-----------------------------------------------------------------------------
#
#  CMake config
#
#  vtzero examples
#
#-----------------------------------------------------------------------------

set(TEST_FILE "${CMAKE_SOURCE_DIR}/test/data/mapbox-streets-v6-14-8714-8017.mvt")

add_executable(vtzero-create vtzero-create.cpp utils.cpp)

# disable building of examples that need getopt lib on MSVC
if(NOT MSVC)
    add_executable(vtzero-filter vtzero-filter.cpp utils.cpp)

    add_test(NAME vtzero-filter-empty
             COMMAND vtzero-filter)
    set_tests_properties(vtzero-filter-empty PROPERTIES
                         PASS_REGULAR_EXPRESSION "^Usage: ")

    add_test(NAME vtzero-filter-help
             COMMAND vtzero-filter -h)
    set_tests_properties(vtzero-filter-help PROPERTIES
                         PASS_REGULAR_EXPRESSION "^vtzero-filter ")

    add_test(NAME vtzero-filter-file
             COMMAND vtzero-filter -o ${CMAKE_CURRENT_BINARY_DIR}/bridges.mvt ${TEST_FILE} bridge)

    add_executable(vtzero-show vtzero-show.cpp utils.cpp)

    add_test(NAME vtzero-show-empty
             COMMAND vtzero-show)
    set_tests_properties(vtzero-show-empty PROPERTIES
                         PASS_REGULAR_EXPRESSION "^Usage: ")

    add_test(NAME vtzero-show-help
             COMMAND vtzero-show -h)
    set_tests_properties(vtzero-show-help PROPERTIES
                         PASS_REGULAR_EXPRESSION "^vtzero-show ")

    add_test(NAME vtzero-show-layers
             COMMAND vtzero-show -l ${TEST_FILE})
    set_tests_properties(vtzero-show-layers PROPERTIES
                         PASS_REGULAR_EXPRESSION "^landuse 78\nwaterway 327\n.*\nwaterway_label 4\n$")

    add_test(NAME vtzero-show-layer-num
             COMMAND vtzero-show ${TEST_FILE} 2)
    set_tests_properties(vtzero-show-layer-num PROPERTIES
                         PASS_REGULAR_EXPRESSION "layer:\n  name: water\n")

    add_test(NAME vtzero-show-layer-name
             COMMAND vtzero-show ${CMAKE_SOURCE_DIR}/test/data/mapbox-streets-v6-14-8714-8017.mvt water)
    set_tests_properties(vtzero-show-layer-name PROPERTIES
                         PASS_REGULAR_EXPRESSION "layer:\n  name: water\n")

endif()


#-----------------------------------------------------------------------------
