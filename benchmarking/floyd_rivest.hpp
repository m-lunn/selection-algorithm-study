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
#include "common.hpp"

namespace sel {

struct FrParams {
    std::size_t cutoff = 16;
    std::size_t thr = 600;
};

template <class T, class Less>
void fr_select(T* a, idx l, idx m, idx k, const FrParams& p, Less& less) {
    while (l < m) {
        if (static_cast<std::size_t>(m - l + 1) <= p.cutoff) {
            insertion_sort(a, l, m, less);
            return;
        }
        if (static_cast<std::size_t>(m - l) > p.thr) {
            const double n = static_cast<double>(m - l + 1);
            const double i = static_cast<double>(k - l + 1);
            const double z = std::log(n);
            const double s = 0.5 * std::exp(2.0 * z / 3.0);
            const double sd =
                0.5 * std::sqrt(z * s * (n - s) / n) * ((i - n / 2.0) < 0 ? -1.0 : 1.0);
            idx nl = std::max(l, static_cast<idx>(k - i * s / n + sd));
            idx nr = std::min(m, static_cast<idx>(k + (n - i) * s / n + sd));
            nl = std::min(nl, k);
            nr = std::max(nr, k);
            fr_select(a, nl, nr, k, p, less);  // puts a good pivot at a[k]
        }
        T x = a[k];
        idx i, j;
        hoare_partition(a, l, m, x, i, j, less);
        narrow(l, m, k, i, j);
    }
}

}  // namespace sel
