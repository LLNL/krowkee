// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#undef NDEBUG

#include <krowkee/hash/hash.hpp>

#include <krowkee/util/tests.hpp>

#if __has_include(<cereal/cereal.hpp>)
#include <check_archive.hpp>
#endif

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <iostream>

typedef krowkee::hash::WangHash    wh_t;
typedef krowkee::hash::MulShift    ms_t;
typedef krowkee::hash::MulAddShift mas_t;

typedef std::chrono::system_clock Clock;
typedef std::chrono::nanoseconds  ns_t;

/**
 * Struct bundling the experiment parameters.
 */
struct parameters_t {
  std::uint64_t count;
  std::uint64_t range;
  std::uint64_t seed;
  bool          verbose;
};

void is_pow2_throw() { krowkee::hash::is_pow2(-5); }

struct pow2_check {
  const char *name() const { return "pow2 check"; }

  template <typename T>
  void check_ceil2(bool &ceil_log2_success, const T val, const T target,
                   const parameters_t &params) const {
    int pow(krowkee::hash::ceil_log2_64(val));
    if (pow != target) {
      ceil_log2_success = false;
    }
    if (params.verbose == true) {
      std::cout << "value " << val << " has ceil log2 " << pow << std::endl;
    }
  }

  void operator()(const parameters_t &params) const {
    std::uint64_t one(1);
    bool          pow2_correct_success = true;
    for (std::uint64_t i(0); i < 64; ++i) {
      if (!krowkee::hash::is_pow2(one << i)) {
        std::cout << "Incorrectly assigned " << (1 << i)
                  << " as a non-power of 2" << std::endl;
        pow2_correct_success = false;
      }
    }
    CHECK_CONDITION(pow2_correct_success, "correct pow2 assignment");

    bool                       pow2_incorrect_success = true;
    std::vector<std::uint64_t> nonpows{3, 13, 71978281, 2821, 29028143};
    for (const std::uint64_t nonpow : nonpows) {
      if (krowkee::hash::is_pow2(nonpow)) {
        std::cout << "Incorrectly assigned " << nonpow << " as a power of 2"
                  << std::endl;
        pow2_incorrect_success = false;
      }
    }
    CHECK_CONDITION(pow2_incorrect_success, "incorrect pow2 assignemnt");

    CHECK_THROWS<std::invalid_argument>(is_pow2_throw, "bad pow2");

    bool             ceil_log2_success = true;
    std::vector<int> targets{1, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
    for (int i(0); i <= (1 << 4); ++i) {
      check_ceil2(ceil_log2_success, i, targets[i], params);
    }
    check_ceil2(ceil_log2_success, std::numeric_limits<std::uint64_t>::max(),
                std::uint64_t(64), params);
    CHECK_CONDITION(ceil_log2_success, "ceil log2");
  }
};

void wh_init(std::uint64_t i) { wh_t hash{i}; }

struct init_check {
  const char *name() const { return "hash initialization check"; }

  void operator()() const {
    for (std::uint64_t i(0); i < 63; ++i) {
      CHECK_DOES_NOT_THROW<std::exception>(wh_init, "good value initialization",
                                           i);
    }
    CHECK_CONDITION(true, "good value initialization");
  }
};

struct empirical_histograms {
  const char *name() const { return "empirical histograms"; }

  template <typename HashType, typename... ARGS>
  void empirical_histogram(const parameters_t params,
                           const double std_dev_range, ARGS &&...args) const {
    std::uint64_t m(
        std::max(krowkee::hash::ceil_pow2_64(params.range), std::uint64_t(2)));
    std::vector<std::uint64_t> hist(m);
    // HashType hash{m, std::forward<ARGS>(args)...};
    auto     start = Clock::now();
    HashType hash{params.range, params.seed, args...};
    for (std::uint64_t i(0); i < params.count; ++i) {
      ++hist[hash(i)];
    }
    online_statistics os{};
    os.push(hist);
    double mean(os.mean()), var(os.variance()), std_dev(os.std_dev()),
        target(std_dev_range * mean);
    if (params.verbose == true) {
      std::cout << "Empirical histogram of " << params.count
                << " elements hashed to 2^" << m << " bins using "
                << HashType::name() << " with state:" << std::endl;
      std::cout
          << "[" << hash.state() << "], ("
          << std::chrono::duration_cast<ns_t>(Clock::now() - start).count()
          << " ns):";

      for (int i(0); i < m; ++i) {
        if (i % 20 == 0) {
          std::cout << "\n\t";
        }
        std::cout << hist[i] << " ";
      }
      std::cout << std::endl;
      std::cout << "\tmean: " << mean << ", variance: " << var
                << ", std dev: " << std_dev
                << ", max target std dev: " << target << std::endl;
    }
    std::stringstream ss;
    ss << HashType::name() << " std dev";
    CHECK_CONDITION(std_dev < target, ss.str());
  }

  void operator()(const parameters_t &params) const {
    empirical_histogram<wh_t>(params, 0.05);
    empirical_histogram<ms_t>(params, 0.01);
    empirical_histogram<mas_t>(params, 0.01);
  }
};

#if __has_include(<cereal/cereal.hpp>)
struct serialize_check {
  const char *name() { return "serialize check"; }

  template <typename HashType, typename... ARGS>
  void serialize(const parameters_t &params, ARGS &&...args) const {
    HashType hash{params.range, params.seed, args...};
    CHECK_ALL_ARCHIVES(hash, HashType::name());
  }

  void operator()(const parameters_t params) const {
    serialize<wh_t>(params);
    serialize<ms_t>(params);
    serialize<mas_t>(params);
  }
};
#endif

void do_experiment(const parameters_t params) {
  print_line();
  print_line();
  std::cout << " Experimenting with " << params.count
            << " insertions into a range of " << params.range
            << " using random seed " << params.seed << std::endl;
  print_line();
  print_line();
  std::cout << std::endl;
  do_test<pow2_check>(params);
  do_test<init_check>();
  do_test<empirical_histograms>(params);
#if __has_include(<cereal/cereal.hpp>)
  do_test<serialize_check>(params);
#endif
  std::cout << std::endl;
}

void print_help(char *exe_name) {
  std::cout << "\nusage:  " << exe_name << "\n"
            << "\t-c, --count <int>  - number of insertions\n"
            << "\t-s, --seed <int>   - random seed\n"
            << "\t-r, --range <int>  - range of hash functors\n"
            << "\t-v, --verbose      - print additional debug information.\n"
            << "\t-h, --help         - print this line and exit\n"
            << std::endl;
}

void parse_args(int argc, char **argv, parameters_t &params) {
  int c;

  while (1) {
    int                  option_index(0);
    static struct option long_options[] = {
        {"count", required_argument, NULL, 'c'},
        {"range", required_argument, NULL, 'r'},
        {"seed", required_argument, NULL, 's'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int curind = optind;
    c = getopt_long(argc, argv, "-:c:r:s:vh", long_options, &option_index);
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
        params.count = std::atoll(optarg);
        break;
      case 'r':
        params.range = std::atoll(optarg);
        break;
      case 's':
        params.seed = std::atoll(optarg);
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
    }
  }
}

int main(int argc, char **argv) {
  std::uint64_t count(10000);
  std::uint64_t range(16);
  std::uint64_t seed(krowkee::hash::default_seed);
  bool          verbose(false);

  parameters_t params{count, range, seed, verbose};

  parse_args(argc, argv, params);

  do_experiment(params);

  return 0;
}
