#pragma once

#include <cstddef>
#include <utility>
#include "partition.hpp"

namespace algorithms {

template <class T, class Less>
void insertion_sort(T* a, std::size_t left, std::size_t right, Less less) {
    for (std::size_t i = left + 1; i <= right; ++i) {
        T value = std::move(a[i]);
        std::size_t j = i;
        while (j > left && less(value, a[j - 1])) {
            a[j] = std::move(a[j - 1]);
            --j;
        }
        a[j] = std::move(value);
    }
}

}  // namespace algorithms
