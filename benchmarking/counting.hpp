// counting.hpp -- comparator wrapper that counts calls.
//
// Used ONLY in separate counted runs, never in timed runs, so the counter's
// memory write cannot distort timings. The counted run uses the same input
// copy and the same RNG seed as the timed run of that repetition, so for every
// algorithm (including the randomized ones) it executes the identical sequence
// of comparisons.
#pragma once
#include <cstdint>

namespace bench {

template <class Less>
struct CountingLess {
    Less less;
    std::uint64_t* counter;
    template <class A, class B>
    bool operator()(const A& x, const B& y) const {
        ++*counter;
        return less(x, y);
    }
};

}  // namespace bench
