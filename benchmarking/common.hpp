// common.hpp -- primitives shared by every selection algorithm.
//
// FAIRNESS DECISION (document this in the report): the implemented selection
// algorithms use the SAME small-case cutoff (insertion sort, 16 by default).
// Their partition routines remain algorithm-specific; std::nth_element and
// std::sort are library baselines and do not expose this cutoff.
//
// Why Hoare-style and not Lomuto or 3-way?
//   * Equal elements are swapped and split between both sides, so all-equal
//     and few-distinct inputs give balanced partitions (no quadratic blowup).
//   * Costs about one comparison per element, so comparison counts line up
//     with the textbook constants (quickselect ~3.4n, Floyd-Rivest ~1.5n for
//     the median). A 3-way partition would need up to 2 comparisons per
//     element when only `less` is available.
//
// Contract of every select function:
//     select(T* a, size_t n, size_t k, Less less)
// On return a[k] holds the k-th smallest element (0-indexed), every element of
// a[0..k) is <= a[k] and every element of a[k+1..n) is >= a[k].
// `less` is a strict weak order: less(x, y) -> bool.
#pragma once

#include <cstddef>
#include <cstdint>

namespace sel {

using idx = std::ptrdiff_t;  // signed: Hoare partition lets j reach l-1

// --- tiny deterministic RNG (xorshift64*), re-seedable per run -------------
inline std::uint64_t& rng_state() {
    static std::uint64_t s = 88172645463325252ULL;
    return s;
}
inline void seed_rng(std::uint64_t seed) {
    rng_state() = seed ? seed : 88172645463325252ULL;
}
inline std::uint64_t next_rand() {
    std::uint64_t& x = rng_state();
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 2685821657736338717ULL;
}
inline idx rand_in(idx lo, idx hi) {  // inclusive bounds
    return lo + static_cast<idx>(next_rand() % static_cast<std::uint64_t>(hi - lo + 1));
}

// Number of times introselect abandoned quickselect for its fallback.
// The harness resets this before a counted run and records it in the CSV.
inline std::uint64_t g_fallbacks = 0;

}  // namespace sel
