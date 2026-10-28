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
#include "algorithms/bfprt.hpp"
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
void bfprt_select(T* a, idx l, idx m, idx k, const BfprtParams& p, Less& less) {
    algorithms::bfprt(a, static_cast<std::size_t>(l), static_cast<std::size_t>(m),
                      static_cast<std::size_t>(k), p.group, p.cutoff, less);
}

}  // namespace sel
