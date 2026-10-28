#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include "bfprt.hpp"

namespace algorithms {

template <class T, class Less>
void introselect(T* a, std::size_t n, std::size_t k, std::size_t cutoff,
                 double depth_multiplier, std::size_t fallback_group, Less less,
                 std::uint64_t seed = 88172645463325252ULL,
                 bool* used_fallback = nullptr) {
    if (n < 2) return;
    std::mt19937_64 rng(seed);
    std::size_t left = 0, right = n - 1;
    std::size_t depth =
        static_cast<std::size_t>(depth_multiplier * std::log2(static_cast<double>(n)));
    while (left < right) {
        if (right - left + 1 <= cutoff) {
            insertion_sort(a, left, right, less);
            return;
        }
        if (depth-- == 0) {
            if (used_fallback) *used_fallback = true;
            bfprt(a, left, right, k, fallback_group, cutoff, less);
            return;
        }
        std::uniform_int_distribution<std::size_t> pick(left, right);
        const T pivot = a[pick(rng)];
        std::ptrdiff_t i, j;
        hoare_partition(a, left, right, pivot, i, j, less);
        if (!narrow(left, right, k, i, j)) return;
    }
}

}  // namespace algorithms
