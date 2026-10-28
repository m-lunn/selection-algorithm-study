#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "algorithms/quickselect.hpp"
#include "generators.hpp"
#include "mcilroy.hpp"
#include "string_inputs.hpp"

namespace {

template <class T>
void print_sample(const std::string& name, const std::vector<T>& values,
                  std::size_t limit = 12) {
    std::cout << name << " (n=" << values.size() << "): [";
    const std::size_t shown = std::min(limit, values.size());
    for (std::size_t i = 0; i < shown; ++i) {
        if (i != 0) std::cout << ", ";
        std::cout << values[i];
    }
    if (shown < values.size()) std::cout << ", ...";
    std::cout << "]\n";
}

}  // namespace

int main() {
    constexpr std::size_t n = 64;
    constexpr std::uint64_t seed = 42;

    print_sample("random", gen::random_permutation(n, seed));
    print_sample("sorted", gen::sorted(n));
    print_sample("reverse", gen::reverse_sorted(n));
    print_sample("nearly_sorted", gen::nearly_sorted(n, 3, seed));
    print_sample("all_equal", gen::all_equal(n));
    print_sample("few_distinct", gen::few_distinct(n, 4, seed));
    print_sample("organ_pipe", gen::organ_pipe(n));
    print_sample("sawtooth", gen::sawtooth(n, 5));

    auto integers = gen::random_permutation(n, seed);
    print_sample("strings(prefix=4)", gen::to_strings(integers, 4));

    constexpr std::size_t adversary_seed = 12345;
    auto adversary = gen::mcilroy_adversary(
        n, n / 2, [=](int* values, std::size_t size, std::size_t k, auto less) {
            algorithms::quickselect(values, 0, size - 1, k, less, 16, adversary_seed);
        });
    print_sample("mcilroy:qs_random(seed=12345)", adversary.input);
    std::cout << "mcilroy comparisons: " << adversary.comparisons << '\n';
}
