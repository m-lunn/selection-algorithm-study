// introselect.hpp -- quickselect with a depth limit and a
// worst-case-linear fallback (Musser-style "introspective" selection).
//
// Behaves like randomized quickselect until the number of partition rounds exceeds
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
#include "algorithms/introselect.hpp"
#include "bfprt.hpp"

namespace sel {

struct IntroParams {
    std::size_t cutoff = 16;
    double depth_mult = 2.0;
    std::size_t fallback_group = 5;
};

template <class T, class Less>
void introselect(T* a, std::size_t n, std::size_t k, const IntroParams& p, Less& less) {
    bool used_fallback = false;
    algorithms::introselect(a, n, k, p.cutoff, p.depth_mult, p.fallback_group, less,
                            sel::next_rand(), &used_fallback);
    if (used_fallback) ++g_fallbacks;
}

}  // namespace sel
