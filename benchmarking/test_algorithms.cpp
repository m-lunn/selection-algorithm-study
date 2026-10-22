// test_algorithms.cpp -- correctness tests for every algorithm configuration.
// Run via `make test` (builds with ASan + UBSan). Run this BEFORE any benchmarking.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "algorithms/registry.hpp"
#include "generators/generators.hpp"
#include "generators/mcilroy.hpp"
#include "generators/string_inputs.hpp"

using gen::Key;

static long g_checks = 0;

template <class T>
static void check(const sel::Algo& algo, const std::vector<T>& in, std::size_t k,
                  const std::string& what) {
    std::vector<T> sorted = in;
    std::sort(sorted.begin(), sorted.end());
    std::vector<T> w = in;
    sel::seed_rng(12345 + k);
    algo(w.data(), w.size(), k, std::less<T>());
    bool ok = !(w[k] < sorted[k]) && !(sorted[k] < w[k]);
    for (std::size_t i = 0; ok && i < k; ++i) ok = !(w[k] < w[i]);
    for (std::size_t i = k + 1; ok && i < w.size(); ++i) ok = !(w[i] < w[k]);
    // must still be a permutation of the input
    if (ok) {
        std::vector<T> s2 = w;
        std::sort(s2.begin(), s2.end());
        ok = (s2 == sorted);
    }
    ++g_checks;
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s on %s (n=%zu, k=%zu)\n", algo.label.c_str(), what.c_str(),
                     in.size(), k);
        std::exit(1);
    }
}

int main() {
    const std::vector<std::string> specs = {
        "qs_random", "qs_random:c=1", "qs_m3", "qs_m3:c=1", "bfprt", "bfprt:g=3",
        "bfprt:g=7:c=1", "bfprt:g=9:c=4", "bfprt:g=2:c=1", "floyd_rivest", "floyd_rivest:thr=20:c=1",
        "floyd_rivest:thr=100:c=8", "introselect", "introselect:c=1:d=0.5", "introselect:c=1:d=0",
        "std_nth", "std_sort"};
    std::vector<sel::Algo> algos;
    for (auto& s : specs) algos.push_back(sel::parse_algo(s));

    // 1. Exhaustive small cases: all n in 1..40, all k, many input families.
    for (std::size_t n = 1; n <= 40; ++n) {
        std::vector<std::pair<std::string, std::vector<Key>>> inputs = {
            {"sorted", gen::sorted(n)},
            {"reverse", gen::reverse_sorted(n)},
            {"all_equal", gen::all_equal(n)},
            {"organ_pipe", gen::organ_pipe(n)},
            {"sawtooth3", gen::sawtooth(n, 3)},
            {"few2", gen::few_distinct(n, 2, 5)},
            {"few3", gen::few_distinct(n, 3, 6)},
        };
        for (int s = 0; s < 4; ++s) inputs.push_back({"random", gen::random_permutation(n, 100 + s)});
        for (auto& [name, in] : inputs)
            for (auto& a : algos)
                for (std::size_t k = 0; k < n; ++k) check(a, in, k, name);
    }

    // 2. Larger inputs (exercise Floyd-Rivest windowing and BFPRT recursion).
    for (std::size_t n : {2000u, 5000u}) {
        std::vector<std::pair<std::string, std::vector<Key>>> inputs = {
            {"random", gen::random_permutation(n, 9)},
            {"sorted", gen::sorted(n)},
            {"organ_pipe", gen::organ_pipe(n)},
            {"all_equal", gen::all_equal(n)},
            {"few5", gen::few_distinct(n, 5, 3)},
            {"nearly_sorted", gen::nearly_sorted(n, 20, 4)},
        };
        for (auto& [name, in] : inputs)
            for (auto& a : algos)
                for (std::size_t k : {std::size_t{0}, n / 100, n / 4, n / 2, (3 * n) / 4, n - 1})
                    check(a, in, k, name);
    }

    // 3. Frozen McIlroy arrays: attack each deterministic algorithm at n=1500,
    //    then replay the frozen array against EVERY algorithm.
    {
        const std::size_t n = 1500, k = n / 2;
        for (auto& target : algos) {
            if (!target.deterministic() || target.cutoff == 1) continue;
            auto res = gen::mcilroy_adversary(n, k, [&](int* a, std::size_t nn, std::size_t kk, auto less) {
                target(a, nn, kk, less);
            });
            for (auto& a : algos) check(a, res.input, k, "mcilroy vs " + target.label);
        }
    }

    // 4. Strings with shared prefixes.
    {
        auto strs = gen::to_strings(gen::random_permutation(500, 8), 30);
        for (auto& a : algos)
            for (std::size_t k : {std::size_t{0}, std::size_t{250}, std::size_t{499}}) check(a, strs, k, "strings");
    }

    // 5. Introselect with depth 0 must actually take its fallback path.
    {
        sel::g_fallbacks = 0;
        auto in = gen::random_permutation(1000, 1);
        check(sel::parse_algo("introselect:d=0"), in, 500, "fallback");
        if (sel::g_fallbacks == 0) { std::fprintf(stderr, "FAIL: fallback never triggered\n"); return 1; }
    }

    std::printf("all algorithm tests passed (%ld checks)\n", g_checks);
    return 0;
}
