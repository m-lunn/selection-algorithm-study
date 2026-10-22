# Build the benchmark suite.   make          -> build/bench, build/gen_test
# (Override the output dir with BUILD=/some/dir if the repo is on a noexec mount.)
#                              make test     -> correctness tests (sanitizers on)
#                              make clean
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
BUILD    ?= build

HDRS := $(wildcard algorithms/*.hpp generators/*.hpp bench/*.hpp)

all: $(BUILD)/bench $(BUILD)/gen_test

$(BUILD)/bench: bench/bench.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DBENCH_FLAGS='"$(CXXFLAGS)"' -I. bench/bench.cpp -o $@

$(BUILD)/gen_test: generators/gen_test.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. generators/gen_test.cpp -o $@

test: tests/test_algorithms.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I. tests/test_algorithms.cpp -o $(BUILD)/test_algorithms
	$(BUILD)/test_algorithms
	$(CXX) -std=c++17 -O1 -g -fsanitize=address,undefined -I. generators/gen_test.cpp -o $(BUILD)/gen_test_san
	$(BUILD)/gen_test_san

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
