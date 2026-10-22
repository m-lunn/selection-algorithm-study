// quickselect.hpp -- classic quickselect with two pivot rules.
//   qs_random : pivot = uniformly random element of the current range
//   qs_m3     : pivot = median of first, middle, last element (deterministic)
//
// Expected comparisons for the median: ~3.39n (random pivot), ~2.75n (median-of-3).
// Worst case O(n^2): for qs_m3, McIlroy-style adversarial or "median-of-3 killer"
// inputs; for qs_random, only with astronomically small probability.
//
// Primary author: <name>
#pragma once
#include "common.hpp"

namespace sel {

template <class T, class Less, class Pick>
void quickselect_driver(T* a, std::size_t n, std::size_t k, std::size_t cutoff,
                        Less& less, Pick pick) {
    if (n < 2) return;
    idx l = 0, m = static_cast<idx>(n) - 1, kk = static_cast<idx>(k);
    while (l < m) {
        if (static_cast<std::size_t>(m - l + 1) <= cutoff) {
            insertion_sort(a, l, m, less);
            return;
        }
        T x = pick(a, l, m);  // pivot VALUE (copy of an element in a[l..m])
        idx i, j;
        hoare_partition(a, l, m, x, i, j, less);
        narrow(l, m, kk, i, j);
    }
}

template <class T, class Less>
void qs_random(T* a, std::size_t n, std::size_t k, std::size_t cutoff, Less& less) {
    quickselect_driver(a, n, k, cutoff, less,
                       [](T* p, idx l, idx m) -> T { return p[rand_in(l, m)]; });
}

template <class T, class Less>
void qs_m3(T* a, std::size_t n, std::size_t k, std::size_t cutoff, Less& less) {
    quickselect_driver(a, n, k, cutoff, less, [&less](T* p, idx l, idx m) -> T {
        return median_of_3(p[l], p[l + (m - l) / 2], p[m], less);
    });
}

}  // namespace sel
