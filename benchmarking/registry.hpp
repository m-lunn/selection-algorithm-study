// registry.hpp -- one callable object per algorithm configuration.
//
// An algorithm is named by a spec string:   name[:key=value[:key=value...]]
//
//   name          keys (defaults)
//   ------------  ----------------------------------------------------------
//   qs_random     c=16   (insertion-sort cutoff)
//   qs_m3         c=16
//   bfprt         g=5 (group size), c=16
//   floyd_rivest  c=16, thr=600 (window used when range size > thr)
//   introselect   c=16, d=2.0 (depth multiplier), fg=5 (fallback group size)
//   std_nth       (std::nth_element, the reference implementation)
//   std_sort      (std::sort, the "just sort it" baseline)
//
// Examples: "bfprt:g=3", "bfprt:g=7:c=32", "floyd_rivest:thr=300", "introselect:d=1".
//
// Algo::operator() is a template, so the same object runs with a plain
// comparator, a counting comparator, or the McIlroy adversary's comparator.
//
// To add an algorithm (e.g. the one from arXiv:1606.00484): write its header,
// add a Kind, a parse case below, and a switch case in operator().
#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "bfprt.hpp"
#include "floyd_rivest.hpp"
#include "introselect.hpp"
#include "quickselect.hpp"

namespace sel {

enum class Kind { QsRandom, QsM3, Bfprt, FloydRivest, Introselect, StdNth, StdSort };

struct Algo {
    Kind kind = Kind::StdNth;
    std::string label;
    std::size_t cutoff = 16, group = 5, thr = 600, fallback_group = 5;
    double depth = 2.0;

    // Deterministic algorithms can be attacked with the McIlroy adversary.
    bool deterministic() const { return kind != Kind::QsRandom; }

    template <class T, class Less>
    void operator()(T* a, std::size_t n, std::size_t k, Less less) const {
        if (n == 0) return;
        switch (kind) {
            case Kind::QsRandom: qs_random(a, n, k, cutoff, less); break;
            case Kind::QsM3: qs_m3(a, n, k, cutoff, less); break;
            case Kind::Bfprt: {
                BfprtParams p;
                p.group = group;
                p.cutoff = cutoff;
                bfprt_select(a, 0, static_cast<idx>(n) - 1, static_cast<idx>(k), p, less);
                break;
            }
            case Kind::FloydRivest: {
                FrParams p;
                p.cutoff = cutoff;
                p.thr = thr;
                fr_select(a, 0, static_cast<idx>(n) - 1, static_cast<idx>(k), p, less);
                break;
            }
            case Kind::Introselect: {
                IntroParams p;
                p.cutoff = cutoff;
                p.depth_mult = depth;
                p.fallback_group = fallback_group;
                introselect(a, n, k, p, less);
                break;
            }
            case Kind::StdNth: std::nth_element(a, a + k, a + n, less); break;
            case Kind::StdSort: std::sort(a, a + n, less); break;
        }
    }
};

inline Algo parse_algo(const std::string& spec) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t p = spec.find(':', start);
        parts.push_back(spec.substr(start, p == std::string::npos ? p : p - start));
        if (p == std::string::npos) break;
        start = p + 1;
    }
    Algo a;
    a.label = spec;
    const std::string& name = parts[0];
    if (name == "qs_random") a.kind = Kind::QsRandom;
    else if (name == "qs_m3") a.kind = Kind::QsM3;
    else if (name == "bfprt") a.kind = Kind::Bfprt;
    else if (name == "floyd_rivest") a.kind = Kind::FloydRivest;
    else if (name == "introselect") a.kind = Kind::Introselect;
    else if (name == "std_nth") a.kind = Kind::StdNth;
    else if (name == "std_sort") a.kind = Kind::StdSort;
    else throw std::invalid_argument("unknown algorithm: " + name);

    for (std::size_t i = 1; i < parts.size(); ++i) {
        std::size_t eq = parts[i].find('=');
        if (eq == std::string::npos)
            throw std::invalid_argument("bad parameter (want key=value): " + parts[i]);
        std::string key = parts[i].substr(0, eq);
        double val = std::stod(parts[i].substr(eq + 1));
        if (key == "c") a.cutoff = static_cast<std::size_t>(val);
        else if (key == "g") a.group = static_cast<std::size_t>(val);
        else if (key == "thr") a.thr = static_cast<std::size_t>(val);
        else if (key == "d") a.depth = val;
        else if (key == "fg") a.fallback_group = static_cast<std::size_t>(val);
        else throw std::invalid_argument("unknown parameter: " + key);
    }
    if (a.group < 2 || a.fallback_group < 2)
        throw std::invalid_argument("group size must be >= 2");
    if (a.cutoff < 1) throw std::invalid_argument("cutoff must be >= 1");
    return a;
}

}  // namespace sel
