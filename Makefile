
PROTOZERO_INCLUDE := ../protozero/include

CXXFLAGS := -std=c++11 -Iinclude -I${PROTOZERO_INCLUDE} -g -Wall -Wextra -Wpedantic -Werror

EXAMPLES := examples/vtzero-create examples/vtzero-filter examples/vtzero-show

TESTS := test/unit_tests

examples: $(EXAMPLES)

tests: $(TESTS)

all: examples tests

examples/utils.o: examples/utils.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

examples/vtzero-create.o: examples/vtzero-create.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

examples/vtzero-create: examples/vtzero-create.o
	$(CXX) $^ -o $@

examples/vtzero-filter.o: examples/vtzero-filter.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

examples/vtzero-filter: examples/vtzero-filter.o examples/utils.o
	$(CXX) $^ -o $@

examples/vtzero-show.o: examples/vtzero-show.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

examples/vtzero-show: examples/vtzero-show.o examples/utils.o
	$(CXX) $^ -o $@

test/unit_tests: test/test_main.o test/tests.o
	$(CXX) $^ -o $@

test/test_main.o: test/test_main.cpp
	$(CXX) $(CXXFLAGS) -Itest/include -c $^ -o $@

test/tests.o: test/tests.cpp
	$(CXX) $(CXXFLAGS) -Itest/include -c $^ -o $@

test: tests
	test/unit_tests

clean:
	rm -f $(EXAMPLES) $(TESTS) examples/*.o test/*.o

.PHONY: clean test

