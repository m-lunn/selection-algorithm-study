// quickselect.hpp -- randomized quickselect adapter.
//
// Primary author: <name>
#pragma once
#include "algorithms/quickselect.hpp"
#include "common.hpp"

namespace sel {

template <class T, class Less>
void qs_random(T* a, std::size_t n, std::size_t k, std::size_t cutoff, Less& less) {
    algorithms::quickselect(a, 0, n - 1, k, less, cutoff, sel::next_rand());
}

}  // namespace sel
