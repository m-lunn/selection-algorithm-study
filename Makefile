# Build the benchmark suite from the repository root.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
BUILD    ?= build

HDRS := $(wildcard algorithms/*.hpp benchmarking/*.hpp generators/*.hpp)

all: $(BUILD)/bench $(BUILD)/gen_test

$(BUILD)/bench: benchmarking/bench.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DBENCH_FLAGS='"$(CXXFLAGS)"' -I. $< -o $@

$(BUILD)/gen_test: generators/gen_test.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. $< -o $@

test: benchmarking/test_algorithms.cpp $(HDRS)
	@mkdir -p $(BUILD)
	$(CXX) -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I. $< -o $(BUILD)/test_algorithms
	$(BUILD)/test_algorithms
	$(CXX) -std=c++17 -O1 -g -fsanitize=address,undefined -I. generators/gen_test.cpp -o $(BUILD)/gen_test_san
	$(BUILD)/gen_test_san

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
