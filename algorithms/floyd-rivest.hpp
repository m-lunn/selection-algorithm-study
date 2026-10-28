#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include "common.hpp"

namespace algorithms {

template <class T, class Less>
void floyd_rivest(T* a, std::size_t left, std::size_t right, std::size_t k,
                  std::size_t threshold, std::size_t cutoff, Less less) {
    while (left < right) {
        if (right - left + 1 <= cutoff) {
            insertion_sort(a, left, right, less);
            return;
        }
        if (right - left > threshold) {
            const double n = static_cast<double>(right - left + 1);
            const double i = static_cast<double>(k - left + 1);
            const double z = std::log(n);
            const double s = 0.5 * std::exp(2.0 * z / 3.0);
            double sd = 0.5 * std::sqrt(z * s * (n - s) / n);
            if (i < n / 2) sd = -sd;
            const auto nl_value = static_cast<std::ptrdiff_t>(k - i * s / n + sd);
            const auto nr_value = static_cast<std::ptrdiff_t>(k + (n - i) * s / n + sd);
            const auto left_signed = static_cast<std::ptrdiff_t>(left);
            const auto right_signed = static_cast<std::ptrdiff_t>(right);
            const std::size_t nl = static_cast<std::size_t>(
                std::max(left_signed, std::min(right_signed, nl_value)));
            const std::size_t nr = static_cast<std::size_t>(
                std::max(left_signed, std::min(right_signed, nr_value)));
            floyd_rivest(a, nl, nr, k, threshold, cutoff, less);
        }
        const T pivot = a[k];
        std::ptrdiff_t i, j;
        hoare_partition(a, left, right, pivot, i, j, less);
        if (!narrow(left, right, k, i, j)) return;
    }
}

}  // namespace algorithms
