// mcilroy.hpp -- McIlroy's "killer adversary" (A Killer Adversary for Quicksort,
// Software: Practice & Experience, 1999) adapted from sorting to selection.
//
// IDEA. The algorithm under attack is run on an array of element *ids*
// 0..n-1. Instead of real values, each id starts "gas" (value undecided). The
// comparator answers comparisons lazily:
//   * gas vs solid  -> the gas element is larger.
//   * gas vs gas    -> freeze one of the two to the next smallest unused value
//                      (the current "candidate pivot" if it is one of the two,
//                      otherwise the other one); the survivor stays gas.
// The "candidate" is the most recent gas element to take part in a comparison,
// which is what a pivot looks like to the adversary. Freezing the pivot to the
// smallest remaining value makes it an extreme, so partitions are lopsided. All
// answers stay consistent with a single total order, which we read off at the
// end: val[id] is the value that id must hold. Putting val[0..n-1] in an array
// gives an input that drives the *same deterministic algorithm* through
// exactly the bad run that was recorded.
//
// CAVEATS (worth writing up in the report):
//   * The attack is specific to the algorithm AND its parameters (pivot rule,
//     cutoff, k, n). The frozen array is just ordinary data for every other
//     algorithm, so replay it against all of them.
//   * Randomized algorithms only get attacked for the exact RNG seed used
//     during generation; changing the seed generally invalidates the attack.
//   * Algorithms that call std::sort / insertion sort on small pieces are fine;
//     the adversary just answers those comparisons too.
//   * Remaining gas elements at the end are given the largest values, in id
//     order, so the output is always a permutation of 0..n-1.
//
// USAGE
//   auto res = gen::mcilroy_adversary(n, k, [](int* a, size_t n, size_t k, auto less) {
//   });
//   std::vector<gen::Key> input = res.input;     // freeze and replay
//
// The select callable must:
//   * take (int* ids, size_t n, size_t k, Less less) and be deterministic,
//   * only read element ids through `less` (never inspect id values for order),
//   * use `less` as a strict weak order returning bool.
//
// Primary author: <name>
#pragma once

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "generators.hpp"

namespace gen {

struct AdversaryResult {
    std::vector<Key> input;         // permutation of 0..n-1 to feed any algorithm
    std::size_t comparisons = 0;    // comparisons the attacked algorithm made
};

template <class Select>
AdversaryResult mcilroy_adversary(std::size_t n, std::size_t k, Select&& select) {
    const Key gas = static_cast<Key>(n);  // larger than any solid value
    std::vector<Key> val(n, gas);
    Key nsolid = 0;
    long candidate = -1;
    std::size_t ncmp = 0;

    auto freeze = [&](int x) { val[x] = nsolid++; };

    auto less = [&](int x, int y) -> bool {
        ++ncmp;
        if (val[x] == gas && val[y] == gas) {
            if (x == candidate) freeze(x);   // pivot becomes the smallest left
            else freeze(y);
        }
        if (val[x] == gas) candidate = x;
        else if (val[y] == gas) candidate = y;
        return val[x] < val[y];
    };

    std::vector<int> ids(n);
    std::iota(ids.begin(), ids.end(), 0);
    select(ids.data(), n, k, less);

    // Resolve whatever never got compared.
    for (std::size_t i = 0; i < n; ++i)
        if (val[i] == gas) val[i] = nsolid++;

    return {std::move(val), ncmp};
}

}  // namespace gen
