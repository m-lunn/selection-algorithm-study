// bfprt.hpp -- median of medians (Blum, Floyd, Pratt, Rivest, Tarjan 1973).
//
// Pivot choice: split the range into groups of g, sort each group with
// insertion sort, move each group's median to the front of the range, then
// recursively select the median of those medians as the pivot. With g = 5 this
// guarantees >= ~30% of the elements on each side, giving O(n) worst case:
//     T(n) <= T(n/5) + T(7n/10) + c*n.
// With g = 3 the recurrence becomes T(n/3) + T(2n/3) + cn = O(n log n).
// Everything is in place: no extra array.
//
// Parameters: group (g, >= 2; odd is the sensible choice), cutoff (insertion
// sort below this size).
//
// Primary author: <name>
#pragma once
#include "common.hpp"

namespace sel {

struct BfprtParams {
    std::size_t group = 5;
    std::size_t cutoff = 16;
};

template <class T, class Less>
void bfprt_select(T* a, idx l, idx m, idx k, const BfprtParams& p, Less& less);

// Returns the index (within a[l..m]) of the median of group medians.
template <class T, class Less>
idx bfprt_pivot_index(T* a, idx l, idx m, const BfprtParams& p, Less& less) {
    const idx g = static_cast<idx>(p.group);
    const idx n = m - l + 1;
    const idx ng = (n + g - 1) / g;
    for (idx gi = 0; gi < ng; ++gi) {
        idx gl = l + gi * g;
        idx gm = std::min(gl + g - 1, m);
        insertion_sort(a, gl, gm, less);
        idx med = gl + (gm - gl) / 2;
        std::swap(a[l + gi], a[med]);  // collect medians at the front
    }
    idx mid = l + (ng - 1) / 2;
    bfprt_select(a, l, l + ng - 1, mid, p, less);
    return mid;
}

template <class T, class Less>
void bfprt_select(T* a, idx l, idx m, idx k, const BfprtParams& p, Less& less) {
    while (l < m) {
        if (static_cast<std::size_t>(m - l + 1) <= p.cutoff) {
            insertion_sort(a, l, m, less);
            return;
        }
        idx pi = bfprt_pivot_index(a, l, m, p, less);
        T x = a[pi];
        idx i, j;
        hoare_partition(a, l, m, x, i, j, less);
        narrow(l, m, k, i, j);
    }
}

}  // namespace sel
