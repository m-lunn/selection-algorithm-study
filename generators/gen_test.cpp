// gen_test.cpp -- sanity checks for the generators.
// Build: g++ -std=c++17 -O2 -Wall -Wextra gen_test.cpp -o gen_test && ./gen_test
#include <cassert>
#include <cstdio>
#include <utility>

#include "generators.hpp"
#include "mcilroy.hpp"
#include "string_inputs.hpp"

static bool is_permutation_of_iota(std::vector<gen::Key> a) {
    std::sort(a.begin(), a.end());
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i] != static_cast<gen::Key>(i)) return false;
    return true;
}

template <class Vec>
static std::size_t count_cmps_on(const Vec& input) {
    std::vector<int> ids(input.size());
    std::iota(ids.begin(), ids.end(), 0);
    std::size_t c = 0;
    std::sort(ids.begin(), ids.end(), [&](int x, int y) {
        ++c;
        return input[x] < input[y];
    });
    return c;
}

int main() {
    const std::size_t n = 5000;

    // Permutation families.
    assert(is_permutation_of_iota(gen::random_permutation(n, 42)));
    assert(is_permutation_of_iota(gen::sorted(n)));
    assert(is_permutation_of_iota(gen::reverse_sorted(n)));
    assert(is_permutation_of_iota(gen::nearly_sorted(n, 50, 7)));

    // Determinism: same (n, seed) -> same array; different seed -> different.
    assert(gen::random_permutation(n, 42) == gen::random_permutation(n, 42));
    assert(gen::random_permutation(n, 42) != gen::random_permutation(n, 43));

    // Duplicate families.
    auto fd = gen::few_distinct(n, 5, 1);
    assert(*std::max_element(fd.begin(), fd.end()) <= 4);
    auto ae = gen::all_equal(n);
    assert(std::all_of(ae.begin(), ae.end(), [](gen::Key x) { return x == 0; }));
    auto op = gen::organ_pipe(9);  // 0 1 2 3 4 3 2 1 0
    assert((op == std::vector<gen::Key>{0, 1, 2, 3, 4, 3, 2, 1, 0}));
    auto sw = gen::sawtooth(7, 3);  // 0 1 2 0 1 2 0
    assert((sw == std::vector<gen::Key>{0, 1, 2, 0, 1, 2, 0}));

    // Registry.
    for (const auto& f : gen::family_names())
        assert(gen::make(f, 100, 1, 4).size() == 100);

    // Strings: order preserved, prefix applied.
    auto ints = gen::random_permutation(1000, 3);
    auto strs = gen::to_strings(ints, 50);
    for (std::size_t i = 0; i + 1 < ints.size(); ++i)
        assert((ints[i] < ints[i + 1]) == (strs[i] < strs[i + 1]));
    assert(strs[0].size() == 50 + gen::base26_width(999));

    // McIlroy adversary: verify that the generated result is a permutation and
    // that replaying a deterministic comparison workload is reproducible.
    const std::size_t k = n / 2;
    auto adv = gen::mcilroy_adversary(
        n, k, [](int* a, std::size_t nn, std::size_t, auto less) {
            std::sort(a, a + nn, less);
        });
    assert(is_permutation_of_iota(adv.input));
    // Replaying the frozen array must reproduce the recorded comparison count
    // (the attacked algorithm is deterministic).
    std::size_t replay = count_cmps_on(adv.input);
    std::printf("n=%zu k=%zu\n", n, k);
    std::printf("  adversary run     : %zu comparisons\n", adv.comparisons);
    std::printf("  replay on frozen  : %zu comparisons\n", replay);
    assert(replay == adv.comparisons);

    std::puts("all generator tests passed");
    return 0;
}
