#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include "common.hpp"

namespace algorithms {

template <class T, class Less>
void quickselect(T* a, std::size_t left, std::size_t right, std::size_t k, Less less,
                 std::size_t cutoff = 16,
                 std::uint64_t seed = 88172645463325252ULL) {
    std::mt19937_64 rng(seed);
    while (left < right) {
        if (right - left + 1 <= cutoff) {
            insertion_sort(a, left, right, less);
            return;
        }
        std::uniform_int_distribution<std::size_t> pick(left, right);
        const std::size_t pivot_index = pick(rng);
        const T pivot = a[pivot_index];
        std::ptrdiff_t i, j;
        hoare_partition(a, left, right, pivot, i, j, less);
        if (!narrow(left, right, k, i, j)) return;
    }
}

}  // namespace algorithms
