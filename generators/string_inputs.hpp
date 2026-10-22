// string_inputs.hpp -- expensive-comparison inputs.
//
// Takes ANY integer family from generators.hpp / mcilroy.hpp and turns each
// integer v into a string:
//
//     prefix_len copies of 'a'  +  v written in fixed-width base 26
//
// Properties:
//   * Order-preserving: string order == integer order (fixed width, most
//     significant digit first), so every int family (sorted, organ pipe,
//     McIlroy-frozen array, duplicates, ...) carries over unchanged and the
//     k-th smallest string corresponds to the k-th smallest integer.
//   * Comparison cost ~ prefix_len: std::string::operator< (memcmp) has to scan
//     the whole shared prefix before reaching the first differing character.
//     prefix_len = 0 is the "cheap string" baseline; sweep 0, 16, 64, 256, 1024.
//   * Memory: n * (prefix_len + width) bytes plus std::string overhead. At
//     prefix_len = 1024 and n = 10^6 that is ~1 GB, so cap n for these runs.
//
// Primary author: <name>
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "generators.hpp"

namespace gen {

// Digits needed to write values 0..max_value in base 26 (at least 1).
inline std::size_t base26_width(Key max_value) {
    std::size_t w = 1;
    while (max_value >= 26) {
        max_value /= 26;
        ++w;
    }
    return w;
}

inline std::vector<std::string> to_strings(const std::vector<Key>& ints,
                                           std::size_t prefix_len) {
    Key mx = 0;
    for (Key v : ints) mx = std::max(mx, v);
    const std::size_t width = base26_width(mx);

    std::vector<std::string> out;
    out.reserve(ints.size());
    for (Key v : ints) {
        std::string s(prefix_len + width, 'a');
        for (std::size_t i = 0; i < width; ++i) {  // fill digits right-to-left
            s[prefix_len + width - 1 - i] = static_cast<char>('a' + v % 26);
            v /= 26;
        }
        out.push_back(std::move(s));
    }
    return out;
}

}  // namespace gen
