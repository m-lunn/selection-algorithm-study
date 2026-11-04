// trace_fr.cpp -- print Floyd-Rivest's top-level rounds on one input.
// Primary author: Ben Hood
//
// Runs the same outer loop as algorithms::floyd_rivest, calling the real window
// select, Hoare partition and narrow, and prints each round's pivot error and
// the range left. Used for report section 5.2.
//
// Build and run from the repository root:
//   g++ -std=c++17 -O2 -I. benchmarking/trace_fr.cpp -o build/trace_fr
//   ./build/trace_fr reverse median          (families: random sorted reverse organ_pipe)
//   ./build/trace_fr reverse median random   (sample the top-level window at random)
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

#include "algorithms/floyd-rivest.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: %s <random|sorted|reverse|organ_pipe> <min|median|max> [random]\n", argv[0]);
        return 1;
    }
    const std::size_t n = 10000000, threshold = 600, cutoff = 16;
    const bool random_window = argc > 3 && std::strcmp(argv[3], "random") == 0;

    std::vector<long long> a(n);
    std::mt19937_64 rng(1);
    for (std::size_t i = 0; i < n; ++i) {
        if (!std::strcmp(argv[1], "sorted") || !std::strcmp(argv[1], "random")) a[i] = (long long)i;
        else if (!std::strcmp(argv[1], "reverse")) a[i] = (long long)(n - 1 - i);
        else a[i] = (long long)std::min(i, n - 1 - i);  // organ_pipe
    }
    if (!std::strcmp(argv[1], "random")) std::shuffle(a.begin(), a.end(), rng);

    std::size_t k = !std::strcmp(argv[2], "min") ? 0 : !std::strcmp(argv[2], "max") ? n - 1 : n / 2;
    std::vector<long long> sorted = a;
    std::nth_element(sorted.begin(), sorted.begin() + k, sorted.end());
    const long long target = sorted[k];

    unsigned long long cmps = 0;
    auto less = [&](long long x, long long y) { ++cmps; return x < y; };

    std::size_t left = 0, right = n - 1;
    for (int round = 1; left < right; ++round) {
        if (right - left + 1 <= cutoff) {
            algorithms::insertion_sort(a.data(), left, right, less);
            std::printf("round %d: insertion sort on the last %zu elements\n", round, right - left + 1);
            break;
        }
        if (right - left > threshold) {  // same window formula as algorithms/floyd-rivest.hpp
            const double nn = double(right - left + 1), i = double(k - left + 1);
            const double z = std::log(nn), s = 0.5 * std::exp(2.0 * z / 3.0);
            double sd = 0.5 * std::sqrt(z * s * (nn - s) / nn);
            if (i < nn / 2) sd = -sd;
            const auto L = (std::ptrdiff_t)left, R = (std::ptrdiff_t)right;
            const std::size_t nl = (std::size_t)std::max(L, std::min(R, (std::ptrdiff_t)(k - i * s / nn + sd)));
            const std::size_t nr = (std::size_t)std::max(L, std::min(R, (std::ptrdiff_t)(k + (nn - i) * s / nn + sd)));
            if (random_window) {  // fill the window with elements drawn from the whole range
                std::uniform_int_distribution<std::size_t> pick(left, right);
                for (std::size_t p = nl; p <= nr; ++p) std::swap(a[p], a[pick(rng)]);
            }
            if (nr - nl < right - left) algorithms::floyd_rivest(a.data(), nl, nr, k, threshold, cutoff, less);
        }
        const long long pivot = a[k];
        std::ptrdiff_t i, j;
        algorithms::hoare_partition(a.data(), left, right, pivot, i, j, less);
        const bool more = algorithms::narrow(left, right, k, i, j);
        std::printf("round %d: pivot error %+lld, range left %zu\n", round, pivot - target,
                    more ? right - left + 1 : (std::size_t)0);
        if (!more) break;
    }
    std::printf("answer %s, comparisons per element %.3f\n", a[k] == target ? "correct" : "WRONG",
                double(cmps) / double(n));
}