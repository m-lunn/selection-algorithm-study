#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include "common.hpp"

namespace algorithms {

template <class T, class Less>
void bfprt(T* a, std::size_t left, std::size_t right, std::size_t k,
           std::size_t group_size, std::size_t cutoff, Less less) {
    if (group_size < 2) {
        throw std::invalid_argument("groupSize must be at least 2");
    }
    while (left < right) {
        const std::size_t length = right - left + 1;
        if (length <= cutoff) {
            insertion_sort(a, left, right, less);
            return;
        }
        const std::size_t groups = (length + group_size - 1) / group_size;
        for (std::size_t group = 0; group < groups; ++group) {
            const std::size_t first = left + group * group_size;
            const std::size_t last = std::min(first + group_size - 1, right);
            std::sort(a + first, a + last + 1, less);
            std::swap(a[left + group], a[first + (last - first) / 2]);
        }
        const std::size_t median = left + (groups - 1) / 2;
        bfprt(a, left, left + groups - 1, median, group_size, cutoff, less);
        const T pivot = a[median];
        std::ptrdiff_t i, j;
        hoare_partition(a, left, right, pivot, i, j, less);
        if (!narrow(left, right, k, i, j)) return;
    }
}

}  // namespace algorithms
