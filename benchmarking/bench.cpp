// bench.cpp -- unified benchmarking harness for selection algorithms.
//
// Every row of the output CSV is one (algorithm, input instance, rank, repetition).
// All algorithms see byte-identical input instances (input_hash proves it).
//
// Build:  make            Run: ./build/bench --help
//
// Primary author: <name>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#ifdef __linux__
#include <sched.h>
#endif

#include "algorithms/registry.hpp"
#include "benchmarking/counting.hpp"
#include "generators/generators.hpp"
#include "generators/mcilroy.hpp"
#include "generators/string_inputs.hpp"

#ifndef BENCH_FLAGS
#define BENCH_FLAGS "unknown"
#endif

namespace fs = std::filesystem;
using gen::Key;
using Clock = std::chrono::steady_clock;

// ------------------------------------------------------------------ options
struct Options {
    std::string type = "int";  // int | string
    std::vector<std::string> families = {"random"};
    std::vector<std::size_t> sizes = {100000};
    std::vector<std::string> ranks = {"median"};
    std::vector<std::string> algos = {"qs_random", "qs_m3", "bfprt", "floyd_rivest",
                                      "introselect", "std_nth"};
    std::vector<std::size_t> prefixes = {0};  // string type only
    int reps = 11;
    int seeds = 3;       // input instances per (family, n) for random families
    int warmup = 1;
    double slow_ms = 3000;  // if the warm-up run is slower than this, do only 1 rep
    std::uint64_t base_seed = 1;
    std::string out = "results/results.csv";
    std::string advdir = "results/adversaries";
    std::size_t max_adv_n = 200000;
    bool time = true, count = true, append = false;
    int pin = -1;
    std::string cmdline;
};

static void usage() {
    std::puts(
        "bench [options]\n"
        "  --type int|string          element type (default int)\n"
        "  --families a,b,...         e.g. random,sorted,reverse,all_equal,organ_pipe,\n"
        "                             few_distinct:2,sawtooth:100,nearly_sorted:0.01,\n"
        "                             mcilroy:<algo spec>   (adversary vs a deterministic algo)\n"
        "  --sizes n1,n2,...          accepts 1e5 notation\n"
        "  --ranks r1,r2,...          min,p1,p25,median,p75,p99,max or q<fraction> e.g. q0.9\n"
        "  --algos a1,a2,...          specs such as bfprt:g=3, floyd_rivest:thr=300 (see registry.hpp)\n"
        "  --prefixes p1,p2,...       shared-prefix lengths (string type)\n"
        "  --reps N --seeds N --warmup N --slow-ms X --seed S\n"
        "  --out FILE --append        CSV output (a FILE.meta.txt is written beside it)\n"
        "  --advdir DIR --max-adv-n N cache dir for frozen adversary arrays / size cap\n"
        "  --no-time | --no-count     skip timed runs / counted runs\n"
        "  --pin CORE                 pin to a CPU core (Linux)\n");
}

static std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, d))
        if (!tok.empty()) out.push_back(tok);
    return out;
}

static Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 0; i < argc; ++i) o.cmdline += std::string(i ? " " : "") + argv[i];
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value for " + a);
            return argv[++i];
        };
        if (a == "--help" || a == "-h") { usage(); std::exit(0); }
        else if (a == "--type") o.type = next();
        else if (a == "--families") o.families = split(next(), ',');
        else if (a == "--sizes") {
            o.sizes.clear();
            for (auto& t : split(next(), ',')) o.sizes.push_back(static_cast<std::size_t>(std::stod(t)));
        } else if (a == "--ranks") o.ranks = split(next(), ',');
        else if (a == "--algos") o.algos = split(next(), ',');
        else if (a == "--prefixes") {
            o.prefixes.clear();
            for (auto& t : split(next(), ',')) o.prefixes.push_back(static_cast<std::size_t>(std::stod(t)));
        } else if (a == "--reps") o.reps = std::stoi(next());
        else if (a == "--seeds") o.seeds = std::stoi(next());
        else if (a == "--warmup") o.warmup = std::stoi(next());
        else if (a == "--slow-ms") o.slow_ms = std::stod(next());
        else if (a == "--seed") o.base_seed = std::stoull(next());
        else if (a == "--out") o.out = next();
        else if (a == "--advdir") o.advdir = next();
        else if (a == "--max-adv-n") o.max_adv_n = static_cast<std::size_t>(std::stod(next()));
        else if (a == "--no-time") o.time = false;
        else if (a == "--no-count") o.count = false;
        else if (a == "--append") o.append = true;
        else if (a == "--pin") o.pin = std::stoi(next());
        else throw std::invalid_argument("unknown option " + a + " (try --help)");
    }
    if (o.type != "int" && o.type != "string") throw std::invalid_argument("--type must be int or string");
    if (!o.time && !o.count) throw std::invalid_argument("nothing to do: --no-time and --no-count");
    return o;
}

// ------------------------------------------------------------- environment
static std::string cpu_model() {
    std::ifstream f("/proc/cpuinfo");
    std::string line;
    while (std::getline(f, line))
        if (line.rfind("model name", 0) == 0) {
            auto p = line.find(':');
            return p == std::string::npos ? line : line.substr(p + 2);
        }
    return "unknown";
}

static void write_meta(const Options& o) {
    std::ofstream m(o.out + ".meta.txt", o.append ? std::ios::app : std::ios::trunc);
    std::time_t t = std::time(nullptr);
    m << "date: " << std::ctime(&t);
    m << "command: " << o.cmdline << "\n";
    m << "cpu: " << cpu_model() << "\n";
    m << "hardware_threads: " << std::thread::hardware_concurrency() << "\n";
    m << "pinned_core: " << o.pin << "\n";
    m << "compiler: " << __VERSION__ << "\n";
    m << "flags: " << BENCH_FLAGS << "\n";
    m << "sizeof(Key): " << sizeof(Key) << "\n---\n";
}

// ---------------------------------------------------------------- utilities
static std::uint64_t fnv(const void* p, std::size_t n, std::uint64_t h) {
    const unsigned char* c = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) { h ^= c[i]; h *= 1099511628211ULL; }
    return h;
}
static std::uint64_t hash_input(const std::vector<Key>& v) {
    std::uint64_t h = 1469598103934665603ULL;
    for (Key x : v) h = fnv(&x, sizeof x, h);
    return h;
}
static std::uint64_t hash_input(const std::vector<std::string>& v) {
    std::uint64_t h = 1469598103934665603ULL;
    for (auto& s : v) h = fnv(s.data(), s.size(), h);
    return h;
}
static std::vector<Key> convert(const std::vector<Key>& v, std::size_t, const Key*) { return v; }
static std::vector<std::string> convert(const std::vector<Key>& v, std::size_t prefix,
                                        const std::string*) {
    return gen::to_strings(v, prefix);
}

static std::size_t resolve_rank(const std::string& name, std::size_t n) {
    for (auto& r : gen::standard_ranks(n))
        if (r.name == name) return r.k;
    if (!name.empty() && name[0] == 'q')
        return static_cast<std::size_t>(std::stod(name.substr(1)) * static_cast<double>(n - 1));
    throw std::invalid_argument("unknown rank: " + name);
}

struct FamilySpec {
    std::string label, name, arg;
    bool is_random() const {
        return name == "random" || name == "nearly_sorted" || name == "few_distinct";
    }
    bool is_adversary() const { return name == "mcilroy"; }
};
static FamilySpec parse_family(const std::string& s) {
    FamilySpec f;
    f.label = s;
    auto p = s.find(':');
    f.name = s.substr(0, p);
    if (p != std::string::npos) f.arg = s.substr(p + 1);
    return f;
}

static std::string sanitize(std::string s) {
    for (char& c : s) if (!isalnum(static_cast<unsigned char>(c))) c = '_';
    return s;
}

// McIlroy adversary against `algo_spec` for size n and rank k, cached on disk so
// the SAME frozen array is replayed against every algorithm (and across runs).
static bool adversary_ints(const Options& o, const std::string& algo_spec, std::size_t n,
                           std::size_t k, std::vector<Key>& out) {
    if (n > o.max_adv_n) return false;
    sel::Algo target = sel::parse_algo(algo_spec);
    if (!target.deterministic())
        throw std::invalid_argument("adversary needs a deterministic algorithm: " + algo_spec);
    fs::create_directories(o.advdir);
    std::string path = o.advdir + "/mcilroy_" + sanitize(algo_spec) + "_n" + std::to_string(n) +
                       "_k" + std::to_string(k) + ".bin";
    if (fs::exists(path)) {
        std::ifstream f(path, std::ios::binary);
        std::uint64_t cnt = 0;
        f.read(reinterpret_cast<char*>(&cnt), sizeof cnt);
        out.resize(cnt);
        f.read(reinterpret_cast<char*>(out.data()), cnt * sizeof(Key));
        if (f && cnt == n) return true;
    }
    std::fprintf(stderr, "  [adversary] generating vs %s, n=%zu, k=%zu ...\n", algo_spec.c_str(), n, k);
    auto res = gen::mcilroy_adversary(n, k, [&](int* a, std::size_t nn, std::size_t kk, auto less) {
        target(a, nn, kk, less);
    });
    out = std::move(res.input);
    std::ofstream f(path, std::ios::binary);
    std::uint64_t cnt = out.size();
    f.write(reinterpret_cast<const char*>(&cnt), sizeof cnt);
    f.write(reinterpret_cast<const char*>(out.data()), cnt * sizeof(Key));
    std::fprintf(stderr, "  [adversary] attacked run made %zu comparisons (%.2f n^2)\n",
                 res.comparisons, static_cast<double>(res.comparisons) / (double(n) * double(n)));
    return true;
}

// ------------------------------------------------------------------- verify
template <class T>
static bool verify(const std::vector<T>& w, std::size_t k, const T& expected) {
    std::less<T> less;
    if (less(w[k], expected) || less(expected, w[k])) return false;
    for (std::size_t i = 0; i < k; ++i) if (less(w[k], w[i])) return false;
    for (std::size_t i = k + 1; i < w.size(); ++i) if (less(w[i], w[k])) return false;
    return true;
}

// ---------------------------------------------------------------- main loop
template <class T>
static int run_suite(const Options& o, std::ofstream& csv) {
    std::vector<sel::Algo> algos;
    for (auto& s : o.algos) algos.push_back(sel::parse_algo(s));
    const T* tag = nullptr;
    std::less<T> plain;
    std::vector<T> base, work;
    std::string cached_key;
    std::uint64_t input_hash = 0;
    const double slow_ns = o.slow_ms * 1e6;

    for (auto& fam_s : o.families) {
        FamilySpec fam = parse_family(fam_s);
        for (std::size_t n : o.sizes) {
            if (n == 0) continue;
            int nseeds = fam.is_random() ? o.seeds : 1;
            for (int sidx = 0; sidx < nseeds; ++sidx) {
                std::uint64_t seed = o.base_seed + static_cast<std::uint64_t>(sidx);
                for (std::size_t prefix : (o.type == "string" ? o.prefixes : std::vector<std::size_t>{0})) {
                    for (auto& rank_name : o.ranks) {
                        std::size_t k = resolve_rank(rank_name, n);

                        // ---- build (or reuse) the input instance
                        std::string key = fam.label + "|" + std::to_string(n) + "|" +
                                          std::to_string(seed) + "|" + std::to_string(prefix) +
                                          (fam.is_adversary() ? "|k" + std::to_string(k) : "");
                        if (key != cached_key) {
                            std::vector<Key> ints;
                            if (fam.is_adversary()) {
                                if (!adversary_ints(o, fam.arg, n, k, ints)) {
                                    std::fprintf(stderr, "skip %s n=%zu (> --max-adv-n)\n", fam.label.c_str(), n);
                                    continue;
                                }
                            } else {
                                double param = fam.arg.empty() ? 0.0 : std::stod(fam.arg);
                                ints = gen::make(fam.name, n, seed, param);
                            }
                            base = convert(ints, prefix, tag);
                            input_hash = hash_input(base);
                            cached_key = key;
                        }
                        work.clear();
                        work.reserve(n);

                        // ---- expected answer for verification
                        std::vector<T> tmp = base;
                        std::nth_element(tmp.begin(), tmp.begin() + k, tmp.end(), plain);
                        const T expected = tmp[k];
                        tmp.clear();
                        tmp.shrink_to_fit();

                        std::fprintf(stderr, "[%s n=%zu seed=%llu prefix=%zu rank=%s k=%zu]\n",
                                     fam.label.c_str(), n, (unsigned long long)seed, prefix,
                                     rank_name.c_str(), k);

                        for (const sel::Algo& algo : algos) {
                            const std::uint64_t ah = std::hash<std::string>{}(algo.label);
                            auto timed_run = [&](std::uint64_t rs) -> double {
                                work = base;  // fresh copy, excluded from timing
                                sel::seed_rng(rs);
                                std::atomic_signal_fence(std::memory_order_seq_cst);
                                auto t0 = Clock::now();
                                algo(work.data(), n, k, plain);
                                auto t1 = Clock::now();
                                std::atomic_signal_fence(std::memory_order_seq_cst);
                                return std::chrono::duration<double, std::nano>(t1 - t0).count();
                            };
                            auto fail = [&](const char* what) {
                                std::fprintf(stderr, "FATAL: %s produced a wrong result (%s) on %s n=%zu k=%zu\n",
                                             algo.label.c_str(), what, fam.label.c_str(), n, k);
                                std::exit(2);
                            };

                            double warm_ns = 0;
                            if (o.time)
                                for (int w = 0; w < o.warmup; ++w)
                                    warm_ns = timed_run(gen::mix_seed(seed, 777 + w));
                            int reps = (o.time && warm_ns > slow_ns) ? 1 : o.reps;

                            for (int r = 0; r < reps; ++r) {
                                std::uint64_t rs = gen::mix_seed(seed * 7919 + r, ah);
                                long long t_ns = -1, cmps = -1;
                                std::uint64_t fb = 0;
                                if (o.time) {
                                    t_ns = static_cast<long long>(timed_run(rs));
                                    if (r == 0 && !verify(work, k, expected)) fail("timed run");
                                }
                                if (o.count) {
                                    work = base;
                                    sel::seed_rng(rs);
                                    std::uint64_t c = 0;
                                    bench::CountingLess<std::less<T>> cl{plain, &c};
                                    sel::g_fallbacks = 0;
                                    algo(work.data(), n, k, cl);
                                    cmps = static_cast<long long>(c);
                                    fb = sel::g_fallbacks;
                                    if (r == 0 && !verify(work, k, expected)) fail("counted run");
                                }
                                csv << algo.label << ',' << o.type << ',' << fam.label << ',' << n << ','
                                    << seed << ',' << prefix << ',' << rank_name << ',' << k << ',' << r
                                    << ',' << t_ns << ',' << cmps << ',' << fb << ',' << std::hex
                                    << input_hash << std::dec << '\n';
                            }
                            csv.flush();
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    try {
        Options o = parse_args(argc, argv);
        if (o.pin >= 0) {
#ifdef __linux__
            cpu_set_t set;
            CPU_ZERO(&set);
            CPU_SET(o.pin, &set);
            if (sched_setaffinity(0, sizeof set, &set) != 0) std::perror("sched_setaffinity");
#else
            std::fprintf(stderr, "--pin is only supported on Linux; ignoring\n");
#endif
        }
        fs::path outp(o.out);
        if (outp.has_parent_path()) fs::create_directories(outp.parent_path());
        bool need_header = !(o.append && fs::exists(o.out) && fs::file_size(o.out) > 0);
        std::ofstream csv(o.out, o.append ? std::ios::app : std::ios::trunc);
        if (!csv) { std::fprintf(stderr, "cannot open %s\n", o.out.c_str()); return 1; }
        if (need_header)
            csv << "algo,type,family,n,seed,prefix,rank,k,rep,time_ns,comparisons,fallbacks,input_hash\n";
        write_meta(o);
        return o.type == "int" ? run_suite<Key>(o, csv) : run_suite<std::string>(o, csv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
