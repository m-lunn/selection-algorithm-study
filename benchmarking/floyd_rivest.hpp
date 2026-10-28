// floyd_rivest.hpp -- Floyd-Rivest SELECT (CACM Algorithm 489, 1975).
//
// Idea: instead of guessing a pivot, recursively select within a small random-
// looking window [newL, newR] around position k whose size is ~ n^(2/3)/2, so
// that a[k] afterwards is very likely extremely close to the true k-th
// smallest. Partitioning the whole range around that pivot then leaves only a
// tiny region to finish. Expected comparisons: n + min(k, n-k) + o(n), i.e.
// ~1.5n for the median (vs ~2n+ for deterministic algorithms).
//
// NOTE: the original is deterministic (the "sample" is a contiguous window, so
// adversarial inputs exist in principle). This implementation follows the
// widely used Wirth-style variant: after the window select, use x = a[k] and a
// Hoare partition shared with the other algorithms.
//
// Parameters: thr (windowing is used only when range size > thr; the original
// uses 600), cutoff (insertion sort below this size).
//
// Primary author: <name>
#pragma once
#include "algorithms/floyd-rivest.hpp"
#include "common.hpp"

namespace sel {

struct FrParams {
    std::size_t cutoff = 16;
    std::size_t thr = 600;
};

template <class T, class Less>
void fr_select(T* a, idx l, idx m, idx k, const FrParams& p, Less& less) {
    algorithms::floyd_rivest(a, static_cast<std::size_t>(l), static_cast<std::size_t>(m),
                             static_cast<std::size_t>(k), p.thr, p.cutoff, less);
}

}  // namespace sel
