// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee/hash/hash.hpp>
#include <krowkee/sketch/interface.hpp>
#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>

struct timer_t {
  timer_t() { reset(); }

  double elapsed() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(now - _start);
    return (double)elapsed.count() / 100000;
  }

  void reset() { _start = std::chrono::steady_clock::now(); }

 private:
  std::chrono::time_point<std::chrono::steady_clock> _start;
};

using Dense32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::Dense, std::int32_t>;
using MapSparse32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::MapSparse32,
                                      std::int32_t>;
using MapPromotable32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::MapPromotable32,
                                      std::int32_t>;
#if __has_include(<boost/container/flat_map.hpp>)
using FlatMapSparse32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::FlatMapSparse32,
                                      std::int32_t>;

using FlatMapPromotable32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::FlatMapPromotable32,
                                      std::int32_t>;
#endif

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

struct parameters_t {
  std::uint64_t count;
  std::uint64_t range_size;
  std::uint64_t domain_size;
  std::uint64_t observation_count;
  std::size_t   compaction_threshold;
  std::size_t   promotion_threshold;
  std::uint64_t seed;
  sketch_type_t sketch_type;
  cmap_type_t   cmap_type;
  bool          verbose;
};

void print_help(char *exe_name) {
  std::cout << "\nusage:  " << exe_name << "\n"
            << "\t-c, --count <int>              - number of insertions\n"
            << "\t-r, --range <int>              - range of sketch transform\n"
            << "\t-d, --domain <int>             - domain of sketch transform\n"
            << "\t-b, --observation_count <int>  - number of sketches to test\n"
            << "\t-o, --compaction-thresh <int>  - compaction threshold\n"
            << "\t-p, --promotion-thresh <int>   - promotion threshold\n"
            << "\t-t, --sketch-type <str>        - sketch type "
               "(cst, sparse_cst, promotable_cst, fwht)\n"
            << "\t-m, --map-type <str>           - map type "
#if __has_include(<boost/container/flat_map.hpp>)
               "(std, boost)\n"
#else
               "(std)\n"
#endif
            << "\t-s, --seed <int>               - random seed\n"
            << "\t-v, --verbose                  - print additional debug "
               "information.\n"
            << "\t-h, --help                     - print this line and exit\n"
            << std::endl;
}

void parse_args(int argc, char **argv, parameters_t &params) {
  int c;

  while (1) {
    int                  option_index(0);
    static struct option long_options[] = {
        {"count", required_argument, NULL, 'c'},
        {"range", required_argument, NULL, 'r'},
        {"domain", required_argument, NULL, 'd'},
        {"observation-count", required_argument, NULL, 'b'},
        {"compaction-thresh", required_argument, NULL, 'o'},
        {"promotion-thresh", required_argument, NULL, 'p'},
        {"sketch-type", required_argument, NULL, 't'},
        {"map-type", required_argument, NULL, 'm'},
        {"seed", required_argument, NULL, 's'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int curind = optind;
    c          = getopt_long(argc, argv, "-:c:r:d:b:o:p:t:m:s:vh", long_options,
                             &option_index);
    if (c == -1) {
      break;
    }

    switch (c) {
      case 'h':
        print_help(argv[0]);
        exit(-1);
        break;
      case 0:
        printf("long option %s", long_options[option_index].name);
        if (optarg) {
          printf(" with arg %s", optarg);
        }
        printf("\n");
        break;
      case 1:
        printf("unused regular argument ignored %s\n", optarg);
        break;
      case 'c':
        params.count = std::atol(optarg);
        break;
      case 'r':
        params.range_size = std::atoll(optarg);
        break;
      case 'd':
        params.domain_size = std::atoll(optarg);
        break;
      case 'b':
        params.observation_count = std::atoll(optarg);
        break;
      case 'o':
        params.compaction_threshold = std::atoll(optarg);
        break;
      case 'p':
        params.promotion_threshold = std::atoll(optarg);
        break;
      case 't':
        params.sketch_type = krowkee::util::get_sketch_type(optarg);
        break;
      case 'm':
        params.cmap_type = krowkee::util::get_cmap_type(optarg);
        break;
      case 's':
        params.seed = std::atol(optarg);
        break;
      case 'v':
        params.verbose = true;
        break;
      case '?':
        if (optopt == 0) {
          printf("Unknown long option \"%s\",", argv[curind]);
        } else {
          printf("Unknown option %c,", optopt);
        }
        printf(" consult %s --help\n", argv[0]);
        break;
      case ':':
        printf("Missing argument for option -%c/--%s\n", optopt,
               long_options[option_index].name);
        break;
      default:
        printf("?? getopt returned character code 0%o ??\n", c);
        break;
    }
  }
}

template <typename KeyType>
void check_bounds(KeyType idx, std::size_t max) {
  if (idx >= max) {
    throw std::out_of_range("insert index too large!");
  }
  if (idx < 0) {
    throw std::out_of_range("insert index too small!");
  }
}

template <typename ValueType>
struct vector_hist_t {
  using value_t = ValueType;
  vector_hist_t(const parameters_t &params) : _hist(params.domain_size) {}

  template <typename KeyType>
  void insert(const KeyType idx) {
    check_bounds(idx, _hist.size());
    _hist[idx]++;
  }

  static std::string name() { return "std::vector"; }

  constexpr std::uint64_t size() const { return _hist.size(); }

 private:
  std::vector<value_t> _hist;
};

template <typename KeyType, typename ValueType>
struct map_hist_t {
  using key_t   = KeyType;
  using value_t = ValueType;
  map_hist_t(const parameters_t &params) : _size(params.domain_size), _hist() {}

  void insert(const key_t idx) {
    check_bounds(idx, _size);
    _hist[idx]++;
  }

  static std::string name() { return "std::map"; }

  constexpr std::uint64_t size() const { return _size; }

 private:
  std::map<key_t, value_t> _hist;
  std::size_t              _size;
};

template <typename KeyType>
struct set_hist_t {
  using key_t = KeyType;
  set_hist_t(const parameters_t &params) : _size(params.domain_size), _hist() {}

  void insert(const key_t idx) {
    check_bounds(idx, _size);
    _hist.insert(idx);
  }

  static std::string      name() { return "std::set"; }
  constexpr std::uint64_t size() const { return _size; }

 private:
  std::set<key_t> _hist;
  std::size_t     _size;
};

std::vector<std::uint64_t> make_samples(const parameters_t &params) {
  std::mt19937                                 gen(params.seed);
  std::uniform_int_distribution<std::uint64_t> dist(0, params.domain_size - 1);

  std::vector<std::uint64_t> samples(params.count);

  for (std::int64_t i(0); i < params.count; ++i) {
    samples[i] = dist(gen);
  }
  return samples;
}

template <typename ContainerType>
double time_insert(const std::vector<std::uint64_t> samples, ContainerType &con,
                   const parameters_t &params) {
  timer_t timer;
  for (const std::uint64_t sample : samples) {
    con.insert(sample);
  }
  return timer.elapsed();
}

template <typename ContainerType>
double time_insert(const std::vector<std::uint64_t> &samples,
                   const parameters_t               &params) {
  ContainerType con(params);
  return time_insert(samples, con, params);
}

template <typename HistType, typename... HistTypes>
void profile_histograms(std::vector<std::pair<std::string, double>> &profiles,
                        const std::vector<std::uint64_t>            &samples,
                        const parameters_t                          &params) {
  double hist_time = time_insert<HistType>(samples, params);
  profiles.push_back({HistType::name(), hist_time});
  if constexpr (sizeof...(HistTypes) > 0) {
    profile_histograms<HistTypes...>(profiles, samples, params);
  }
  return;
}

template <typename... HistTypes>
std::vector<std::pair<std::string, double>> profile_histograms(
    const std::vector<std::uint64_t> &samples, const parameters_t &params) {
  std::vector<std::pair<std::string, double>> profiles;
  if constexpr (sizeof...(HistTypes) > 0) {
    profile_histograms<HistTypes...>(profiles, samples, params);
  }
  return profiles;
}

template <typename SketchType, typename... SketchTypes>
void profile_sketches(std::vector<std::pair<std::string, double>> &profiles,
                      const std::vector<std::uint64_t>            &samples,
                      const parameters_t                          &params) {
  typedef SketchType            sk_t;
  typedef typename sk_t::sf_t   sf_t;
  typedef std::shared_ptr<sf_t> sf_ptr_t;

  sf_ptr_t sf_ptr(std::make_shared<sf_t>(params.seed));
  sk_t     sk(sf_ptr, params.compaction_threshold, params.promotion_threshold);
  double   sk_time = time_insert(samples, sk, params);
  profiles.push_back({sk_t::full_name(), sk_time});
  if constexpr (sizeof...(SketchTypes) > 0) {
    profile_sketches<SketchTypes...>(profiles, samples, params);
  }
  return;
}

template <typename... SketchTypes>
std::vector<std::pair<std::string, double>> profile_sketches(
    const std::vector<std::uint64_t> &samples, const parameters_t &params) {
  std::vector<std::pair<std::string, double>> profiles;
  if constexpr (sizeof...(SketchTypes) > 0) {
    profile_sketches<SketchTypes...>(profiles, samples, params);
  }
  return profiles;
}

void benchmark(const parameters_t &params) {
  std::vector<std::uint64_t> samples(make_samples(params));

#if __has_include(<boost/container/flat_map.hpp>)
  auto sk_profiles =
      profile_sketches<Dense32CountSketch, MapSparse32CountSketch,
                       MapPromotable32CountSketch, FlatMapSparse32CountSketch,
                       FlatMapPromotable32CountSketch>(samples, params);
#else
  auto sk_profiles =
      profile_sketches<Dense32CountSketch, MapSparse32CountSketch,
                       MapPromotable32CountSketch>(samples, params);
#endif

  auto hist_profiles =
      profile_histograms<vector_hist_t<std::uint64_t>,
                         set_hist_t<std::uint64_t>,
                         map_hist_t<std::uint64_t, std::uint64_t>>(samples,
                                                                   params);
  for (const auto sk_pair : sk_profiles) {
    std::cout << sk_pair.first << " " << sk_pair.second << "s" << std::endl;
    for (const auto hist_pair : hist_profiles) {
      std::cout << "\tvs " << hist_pair.first << " " << hist_pair.second
                << "s (" << (sk_pair.second / hist_pair.second) << "x slowdown)"
                << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  uint64_t      count(10000);
  std::uint64_t range_size(512);
  std::uint64_t domain_size(4096);
  std::uint64_t observation_count(16);
  std::uint64_t seed(krowkee::hash::default_seed);
  std::size_t   compaction_threshold(10);
  std::size_t   promotion_threshold(8);
  sketch_type_t sketch_type(sketch_type_t::cst);
  cmap_type_t   cmap_type(cmap_type_t::std);
  bool          verbose(false);
  bool          do_all(argc == 1);

  parameters_t params{count,
                      range_size,
                      domain_size,
                      observation_count,
                      compaction_threshold,
                      promotion_threshold,
                      seed,
                      sketch_type,
                      cmap_type,
                      verbose};

  parse_args(argc, argv, params);

  benchmark(params);
  return 0;
}