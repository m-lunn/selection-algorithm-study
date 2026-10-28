#pragma once

#include <cstddef>
#include <utility>

namespace algorithms {

template <class T, class Less>
void hoare_partition(T* a, std::size_t left, std::size_t right, const T& pivot,
                     std::ptrdiff_t& i, std::ptrdiff_t& j, Less less) {
    i = static_cast<std::ptrdiff_t>(left);
    j = static_cast<std::ptrdiff_t>(right);
    while (i <= j) {
        while (less(a[i], pivot)) ++i;
        while (less(pivot, a[j])) --j;
        if (i <= j) {
            std::swap(a[i], a[j]);
            ++i;
            --j;
        }
    }
}

inline bool narrow(std::size_t& left, std::size_t& right, std::size_t k,
                   std::ptrdiff_t i, std::ptrdiff_t j) {
    const auto target = static_cast<std::ptrdiff_t>(k);
    if (j < target && target >= i)
        left = static_cast<std::size_t>(i);
    if (target < i && target <= j)
        right = static_cast<std::size_t>(j);
    return !(j < target && target < i);
}

}  // namespace algorithms
