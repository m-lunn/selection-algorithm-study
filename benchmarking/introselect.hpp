// introselect.hpp -- quickselect (median-of-3) with a depth limit and a
// worst-case-linear fallback (Musser-style "introspective" selection).
//
// Behaves like qs_m3 until the number of partition rounds exceeds
//     depth_mult * log2(n)
// and then finishes the current range with BFPRT. Gives O(n) worst case with
// quickselect's typical speed.
//
// NOTE: libstdc++'s std::nth_element is also an introselect, but its fallback
// is heap-select (O(n log n) worst case), not BFPRT, and it uses an unguarded
// partition and an insertion-sort cutoff of 3. Compare against std_nth rather
// than assuming they are the same algorithm.
//
// Each time the fallback fires, sel::g_fallbacks is incremented so the harness
// can report how often the safety net was actually needed.
//
// Parameters: cutoff, depth_mult (default 2.0), fallback group size.
//
// Primary author: <name>
#pragma once
#include "bfprt.hpp"

namespace sel {

struct IntroParams {
    std::size_t cutoff = 16;
    double depth_mult = 2.0;
    std::size_t fallback_group = 5;
};

template <class T, class Less>
void introselect(T* a, std::size_t n, std::size_t k, const IntroParams& p, Less& less) {
    if (n < 2) return;
    idx l = 0, m = static_cast<idx>(n) - 1, kk = static_cast<idx>(k);
    idx depth = static_cast<idx>(p.depth_mult * std::log2(static_cast<double>(n)));
    while (l < m) {
        if (static_cast<std::size_t>(m - l + 1) <= p.cutoff) {
            insertion_sort(a, l, m, less);
            return;
        }
        if (depth-- <= 0) {
            ++g_fallbacks;
            BfprtParams bp;
            bp.group = p.fallback_group;
            bp.cutoff = p.cutoff;
            bfprt_select(a, l, m, kk, bp, less);
            return;
        }
        T x = median_of_3(a[l], a[l + (m - l) / 2], a[m], less);
        idx i, j;
        hoare_partition(a, l, m, x, i, j, less);
        narrow(l, m, kk, i, j);
    }
}

}  // namespace sel
