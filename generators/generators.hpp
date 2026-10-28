// generators.hpp -- input generators for the selection benchmark.
//
// Every generator is a pure function of (n, seed, parameters), so the same
// instance can be regenerated anywhere. Values are int64_t. Unless noted, the
// output is a permutation of 0..n-1 (so the k-th smallest element is exactly k);
// families with duplicates say so explicitly.
//
// Primary author: <name>
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gen {

using Key = std::int64_t;
using Rng = std::mt19937_64;

// splitmix64: mixes (seed, stream) into one well-spread 64-bit seed, so that
// seed=1 and seed=2 give unrelated sequences and different generators that use
// the same seed don't produce correlated data.
inline std::uint64_t mix_seed(std::uint64_t seed, std::uint64_t stream = 0) {
    std::uint64_t z = seed + 0x9E3779B97F4A7C15ULL * (stream + 1);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

inline Rng make_rng(std::uint64_t seed, std::uint64_t stream = 0) {
    return Rng(mix_seed(seed, stream));
}

// ---------------------------------------------------------------------------
// Permutation families (output is a permutation of 0..n-1, no duplicates)
// ---------------------------------------------------------------------------

inline std::vector<Key> random_permutation(std::size_t n, std::uint64_t seed) {
    std::vector<Key> a(n);
    std::iota(a.begin(), a.end(), Key{0});
    Rng rng = make_rng(seed, 1);
    std::shuffle(a.begin(), a.end(), rng);
    return a;
}

inline std::vector<Key> sorted(std::size_t n) {
    std::vector<Key> a(n);
    std::iota(a.begin(), a.end(), Key{0});
    return a;
}

inline std::vector<Key> reverse_sorted(std::size_t n) {
    std::vector<Key> a(n);
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<Key>(n - 1 - i);
    return a;
}

// Sorted, then `swaps` random transpositions. Models "almost sorted" data
// (e.g. an appended log). swaps = 0 gives sorted input.
inline std::vector<Key> nearly_sorted(std::size_t n, std::size_t swaps,
                                      std::uint64_t seed) {
    std::vector<Key> a = sorted(n);
    if (n < 2) return a;
    Rng rng = make_rng(seed, 2);
    std::uniform_int_distribution<std::size_t> d(0, n - 1);
    for (std::size_t s = 0; s < swaps; ++s) std::swap(a[d(rng)], a[d(rng)]);
    return a;
}

// ---------------------------------------------------------------------------
// Families with duplicates
// ---------------------------------------------------------------------------

// Every element equal. Stress test for two-way partitioning (Lomuto/Hoare
// without special handling can go quadratic or degenerate here).
inline std::vector<Key> all_equal(std::size_t n, Key value = 0) {
    return std::vector<Key>(n, value);
}

// n values drawn uniformly from {0..d-1}. d=1 is all_equal, d=2 is the
// two-value case, d ~ sqrt(n) gives moderate duplication.
inline std::vector<Key> few_distinct(std::size_t n, std::size_t d,
                                     std::uint64_t seed) {
    if (d == 0) throw std::invalid_argument("few_distinct: d must be >= 1");
    std::vector<Key> a(n);
    Rng rng = make_rng(seed, 3);
    std::uniform_int_distribution<std::uint64_t> dist(0, d - 1);
    for (auto& x : a) x = static_cast<Key>(dist(rng));
    return a;
}

// Organ pipe: 0,1,2,...,peak,...,2,1,0 (each value except the peak appears
// twice).
inline std::vector<Key> organ_pipe(std::size_t n) {
    std::vector<Key> a(n);
    for (std::size_t i = 0; i < n; ++i)
        a[i] = static_cast<Key>(std::min(i, n - 1 - i));
    return a;
}

// Sawtooth: i mod period, i.e. repeated ascending runs. period=n gives sorted,
// period=1 gives all_equal.
inline std::vector<Key> sawtooth(std::size_t n, std::size_t period) {
    if (period == 0) throw std::invalid_argument("sawtooth: period must be >= 1");
    std::vector<Key> a(n);
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<Key>(i % period);
    return a;
}

// ---------------------------------------------------------------------------
// Registry: lets the harness build inputs from a family name (e.g. from a CSV
// row or command-line flag).
//
//   family        param meaning
//   ------------  ------------------------------------------
//   random        (unused)
//   sorted        (unused)
//   reverse       (unused)
//   nearly_sorted fraction of n used as number of swaps (e.g. 0.01)
//   all_equal     (unused)
//   few_distinct  number of distinct values d (>= 1)
//   organ_pipe    (unused)
//   sawtooth      period (>= 1)
//
// The McIlroy adversary is in mcilroy.hpp since it needs the algorithm.
// ---------------------------------------------------------------------------

inline const std::vector<std::string>& family_names() {
    static const std::vector<std::string> names = {
        "random",     "sorted",       "reverse",  "nearly_sorted",
        "all_equal",  "few_distinct", "organ_pipe", "sawtooth"};
    return names;
}

inline std::vector<Key> make(const std::string& family, std::size_t n,
                             std::uint64_t seed, double param = 0.0) {
    if (family == "random") return random_permutation(n, seed);
    if (family == "sorted") return sorted(n);
    if (family == "reverse") return reverse_sorted(n);
    if (family == "nearly_sorted")
        return nearly_sorted(n, static_cast<std::size_t>(param * n), seed);
    if (family == "all_equal") return all_equal(n);
    if (family == "few_distinct")
        return few_distinct(n, static_cast<std::size_t>(param), seed);
    if (family == "organ_pipe") return organ_pipe(n);
    if (family == "sawtooth") return sawtooth(n, static_cast<std::size_t>(param));
    throw std::invalid_argument("unknown input family: " + family);
}

// ---------------------------------------------------------------------------
// Target ranks (0-indexed k, i.e. the (k+1)-th smallest). Algorithms behave
// differently by rank, e.g. Floyd-Rivest at k=0 vs k=n/2, so sweep these.
// ---------------------------------------------------------------------------

struct NamedRank {
    std::string name;
    std::size_t k;
};

inline std::vector<NamedRank> standard_ranks(std::size_t n) {
    if (n == 0) return {};
    return {{"min", 0},
            {"p1", n / 100},
            {"p25", n / 4},
            {"median", n / 2},
            {"p75", (3 * n) / 4},
            {"p99", (99 * n) / 100},
            {"max", n - 1}};
}

}  // namespace gen
