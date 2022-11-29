// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/hash.hpp>
#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

struct parameters_t {
  std::uint64_t count;
  std::uint64_t range_size;
  std::uint64_t domain_size;
  std::uint64_t observation_count;
  std::size_t   compaction_threshold;
  std::size_t   promotion_threshold;
  std::size_t   iterations;
  std::uint64_t seed;
  sketch_type_t sketch_type;
  cmap_type_t   cmap_type;
  bool          verbose;
};

std::ostream &operator<<(std::ostream &os, const parameters_t &params) {
  os << "parameters:";
  os << "\n\tcount: " << params.count;
  os << "\n\trange_size: " << params.range_size;
  os << "\n\tdomain_size: " << params.domain_size;
  os << "\n\tobservation_count: " << params.observation_count;
  os << "\n\tcompaction_threshold: " << params.compaction_threshold;
  os << "\n\tpromotion_threshold: " << params.promotion_threshold;
  os << "\n\titerations: " << params.iterations;
  os << "\n\tseed: " << params.seed;
  os << "\n\tverbose: " << params.verbose;
  return os;
}

void print_help(char *exe_name) {
  std::cout << "\nusage:  " << exe_name << "\n"
            << "\t-c, --count <int>              - number of insertions\n"
            << "\t-r, --range <int>              - range of sketch transform\n"
            << "\t-d, --domain <int>             - domain of sketch transform\n"
            << "\t-b, --observation_count <int>  - number of sketches to test\n"
            << "\t-o, --compaction-thresh <int>  - compaction threshold\n"
            << "\t-p, --promotion-thresh <int>   - promotion threshold\n"
            << "\t-i, --iterations <int>         - number of iterations\n"
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

parameters_t parse_args(int argc, char **argv) {
  uint64_t      count(10000);
  std::uint64_t range_size(8);
  std::uint64_t domain_size(4096);
  std::uint64_t observation_count(16);
  std::uint64_t seed(krowkee::hash::default_seed);
  std::size_t   compaction_threshold(10);
  std::size_t   promotion_threshold(8);
  std::size_t   iterations(10);
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
                      iterations,
                      seed,
                      sketch_type,
                      cmap_type,
                      verbose};

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
        {"iterations", required_argument, NULL, 'i'},
        {"sketch-type", required_argument, NULL, 't'},
        {"map-type", required_argument, NULL, 'm'},
        {"seed", required_argument, NULL, 's'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int curind = optind;
    c = getopt_long(argc, argv, "-:c:r:d:b:o:p:i:t:m:s:vh", long_options,
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
      case 'i':
        params.iterations = std::atoll(optarg);
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
  return params;
}
