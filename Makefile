
PROTOZERO_INCLUDE := ../protozero/include

COMPILE_FLAGS := -std=c++11 -Iinclude -I${PROTOZERO_INCLUDE} -g -Wall -Wextra -Wpedantic -Werror

EXAMPLES := examples/vtzero-create examples/vtzero-filter examples/vtzero-show

HPP_FILES := include/vtzero/builder.hpp \
             include/vtzero/exception.hpp \
             include/vtzero/feature.hpp \
             include/vtzero/geometry.hpp \
             include/vtzero/index.hpp \
             include/vtzero/layer.hpp \
             include/vtzero/property_value.hpp \
             include/vtzero/types.hpp \
             include/vtzero/value_view.hpp \
             include/vtzero/vector_tile.hpp \
             include/vtzero/version.hpp

DOC_FILES := doc/advanced.md

TESTS := test/unit_tests

examples: $(EXAMPLES)

tests: $(TESTS)

all: examples tests

examples/utils.o: examples/utils.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -c $^ -o $@

examples/vtzero-create.o: examples/vtzero-create.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -c $^ -o $@

examples/vtzero-create: examples/vtzero-create.o
	$(CXX) $(LDFLAGS) $^ -o $@

examples/vtzero-filter.o: examples/vtzero-filter.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -c $^ -o $@

examples/vtzero-filter: examples/vtzero-filter.o examples/utils.o
	$(CXX) $(LDFLAGS) $^ -o $@

examples/vtzero-show.o: examples/vtzero-show.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -c $^ -o $@

examples/vtzero-show: examples/vtzero-show.o examples/utils.o
	$(CXX) $(LDFLAGS) $^ -o $@

test/unit_tests: test/test_main.o test/fixture_tests.o test/tests.o
	$(CXX) $(LDFLAGS) $^ -o $@

test/test_main.o: test/test_main.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -Itest/include -c $^ -o $@

test/tests.o: test/tests.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -Itest/include -c $^ -o $@

test/fixture_tests.o: test/fixture_tests.cpp
	$(CXX) $(CXXFLAGS) $(COMPILE_FLAGS) -Itest/include -c $^ -o $@

test: tests
	test/unit_tests

doc: doc/Doxyfile README.md $(DOC_FILES) $(HPP_FILES)
	doxygen doc/Doxyfile

clean:
	rm -f $(EXAMPLES) $(TESTS) examples/*.o test/*.o

.PHONY: clean test doc

