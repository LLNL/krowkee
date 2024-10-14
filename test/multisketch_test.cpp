// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee/sketch/interface.hpp>
#include <krowkee/stream/interface.hpp>

#include <krowkee/hash/hash.hpp>

#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>
#include <krowkee/util/tests.hpp>

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <cstring>

using sketch_impl_type = krowkee::util::sketch_impl_type;
using cmap_impl_type   = krowkee::util::cmap_impl_type;

template <std::size_t RangeSize>
using MultiLocalDense32CountSketch = krowkee::stream::MultiLocalCountSketch<
    krowkee::sketch::Dense, std::uint64_t, std::int32_t, RangeSize>;

template <std::size_t RangeSize>
using MultiLocalMapSparse32CountSketch = krowkee::stream::MultiLocalCountSketch<
    krowkee::sketch::MapSparse32, std::uint64_t, std::int32_t, RangeSize>;
template <std::size_t RangeSize>
using MultiLocalMapPromotable32CountSketch =
    krowkee::stream::MultiLocalCountSketch<krowkee::sketch::MapPromotable32,
                                           std::uint64_t, std::int32_t,
                                           RangeSize>;

#if __has_include(<boost/container/flat_map.hpp>)
template <std::size_t RangeSize>
using MultiLocalFlatMapSparse32CountSketch =
    krowkee::stream::MultiLocalCountSketch<krowkee::sketch::FlatMapSparse32,
                                           std::uint64_t, std::int32_t,
                                           RangeSize>;

template <std::size_t RangeSize>
using MultiLocalFlatMapPromotable32CountSketch =
    krowkee::stream::MultiLocalCountSketch<krowkee::sketch::FlatMapPromotable32,
                                           std::uint64_t, std::int32_t,
                                           RangeSize>;
#endif

template <std::size_t RangeSize>
using MultiLocalDense32FWHT =
    krowkee::stream::MultiLocalFWHT<std::uint64_t, std::int32_t, RangeSize>;

/**
 * Struct bundling the experiment parameters.
 */
struct Parameters {
  std::uint64_t    count;
  std::uint64_t    range_size;
  std::size_t      compaction_threshold;
  std::size_t      promotion_threshold;
  std::uint64_t    seed;
  sketch_impl_type sketch_impl;
  cmap_impl_type   cmap_impl;
  bool             verbose;
};

/**
 * Verify that krowkee::Sketch::Multi correctly handles multiple SketchType
 * objects. Not sure how much more thorough I should be.
 */
template <typename MultiType, template <typename> class MakePtrFunc>
struct multi_ingest_check {
  using multi_type         = MultiType;
  using transform_type     = typename multi_type::transform_type;
  using transform_ptr_type = typename multi_type::transform_ptr_type;
  using make_ptr_type      = MakePtrFunc<transform_type>;

  std::string name() const {
    std::stringstream ss;
    ss << multi_type::name() << " good merges";
    return ss.str();
  }

  void operator()(const Parameters &params) const {
    make_ptr_type      _make_ptr = make_ptr_type();
    transform_ptr_type transform_ptr(_make_ptr(params.seed));
    multi_type         dsk(transform_ptr, params.compaction_threshold,
                           params.promotion_threshold);
    for (int i(0); i < params.count; i++) {
      dsk.insert(0, i);
      dsk.insert(1, i);
      dsk.insert(2, i + 1000000);
      dsk.insert(3, i);
      dsk.insert(3, i + 1000000);
    }
    dsk.compactify();

    {
      bool equal_ingest = dsk.at(0) == dsk.at(1);
      CHECK_CONDITION(equal_ingest == true, "equal ingest");
    }
    {
      bool unequal_ingest = dsk.at(0) != dsk.at(2);
      CHECK_CONDITION(unequal_ingest == true, "unequal ingest");
    }
    {
      bool unequal_merge = dsk.at(3) != dsk.at(1);
      CHECK_CONDITION(unequal_merge == true, "incorrect merge");
    }
    {
      // std::cout << dsk.at(1).sk << std::endl;
      // std::cout << dsk.at(2).sk << std::endl;
      // std::cout << (dsk.at(1).sk + dsk.at(2).sk) << std::endl;
      // std::cout << dsk.at(3).sk << std::endl;
      bool equal_merge = dsk.at(3).sk == (dsk.at(1).sk + dsk.at(2).sk);
      CHECK_CONDITION(equal_merge == true, "correct merge");
    }
    {
      bool correct_size = dsk.size() == 4;
      CHECK_CONDITION(correct_size == true, "size query");
    }
    {
      bool correct_degree = dsk.at(0).count == params.count;
      CHECK_CONDITION(correct_degree == true, "degree check");
    }
    {
      bool equal_access = dsk[0] == dsk[1];
      CHECK_CONDITION(equal_access == true, "accesor equality agreement");
    }
    {
      auto count_inserts = [](const std::uint64_t sum, const auto &vdata) {
        return sum + vdata.second.count;
      };
      std::uint64_t empirical_count(std::accumulate(
          std::begin(dsk), std::end(dsk), std::uint64_t(0), count_inserts));
      {
        bool accumulate_check = empirical_count == 5 * params.count;
        if (accumulate_check == false) {
          std::cout << "empirical count: " << empirical_count << std::endl;
        }
        CHECK_CONDITION(accumulate_check == true, "accumulate check");
      }
    }
    {
      const auto sk(dsk.at(0).sk);
      int        sum(accumulate(sk, 0.0));
      double     rel_mag((double)sum / params.count);
      if (params.verbose == true) {
        std::cout << "\t" << ((sk.is_sparse() == true) ? "sparse" : "dense")
                  << std::endl;
        std::cout << "\t" << sk << std::endl;
        std::cout << "\tregister sum (should be near zero): " << sum
                  << ", relative magnitude: " << rel_mag << std::endl;
      }
      CHECK_CONDITION(rel_mag < 0.1,
                      "register sum relative magnitude near zero");
    }

    {
      multi_type dsk2(dsk);
      bool       partial_copy_success = dsk.at(0) == dsk2.at(0);
      CHECK_CONDITION(partial_copy_success == true, "partial copy constructor");
      bool copy_success = dsk == dsk2;
      CHECK_CONDITION(copy_success == true, "full copy constructor");
      dsk2.insert(0, 939);
      bool change_success = dsk != dsk2;
      CHECK_CONDITION(change_success == true, "full disagreement");
    }
  }
};

void print_help(char *exe_name) {
  std::cout << "\nusage:  " << exe_name << "\n"
            << "\t-c, --count <int>              - number of insertions\n"
            << "\t-s, --seed <int>               - random seed\n"
            << "\t-r, --range <int>              - range of sketch transform\n"
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
            << "\t-v, --verbose                  - print additional debug "
               "information.\n"
            << "\t-h, --help                     - print this line and exit\n"
            << std::endl;
}

void parse_args(int argc, char **argv, Parameters &params) {
  int c;

  while (1) {
    int                  option_index(0);
    static struct option long_options[] = {
        {"count", required_argument, NULL, 'c'},
        {"range", required_argument, NULL, 'r'},
        {"compaction-thresh", required_argument, NULL, 'o'},
        {"promotion-thresh", required_argument, NULL, 'p'},
        {"sketch-type", required_argument, NULL, 't'},
        {"map-type", required_argument, NULL, 'm'},
        {"seed", required_argument, NULL, 's'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int curind = optind;
    c          = getopt_long(argc, argv, "-:c:r:o:p:t:m:s:vh", long_options,
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
      case 'o':
        params.compaction_threshold = std::atoll(optarg);
        break;
      case 'p':
        params.promotion_threshold = std::atoll(optarg);
        break;
      case 't':
        params.sketch_impl = krowkee::util::get_sketch_impl_type(optarg);
        break;
      case 'm':
        params.cmap_impl = krowkee::util::get_cmap_impl_type(optarg);
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

/**
 * Execute the battery of tests for the given sketch functor.
 */
template <typename MultiType, template <typename> class MakePtrFunc>
void perform_tests(const Parameters &params) {
  using multi_type     = MultiType;
  using sketch_type    = typename multi_type::sketch_type;
  using transform_type = typename multi_type::transform_type;

  print_line();
  print_line();
  std::cout << "Testing " << multi_type::full_name() << std::endl;
  print_line();
  print_line();

  std::cout << std::endl << std::endl;

  do_test<multi_ingest_check<multi_type, MakePtrFunc>>(params);
}

template <std::size_t RangeSize>
void choose_local_tests_sized(const Parameters &params) {
  if (params.sketch_impl == sketch_impl_type::cst) {
    perform_tests<MultiLocalDense32CountSketch<RangeSize>, make_shared_functor>(
        params);
  } else if (params.sketch_impl == sketch_impl_type::sparse_cst) {
    if (params.cmap_impl == cmap_impl_type::std) {
      perform_tests<MultiLocalMapSparse32CountSketch<RangeSize>,
                    make_shared_functor>(params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_impl == cmap_impl_type::boost) {
      perform_tests<MultiLocalFlatMapSparse32CountSketch<RangeSize>,
                    make_shared_functor>(params);
#endif
    }
  } else if (params.sketch_impl == sketch_impl_type::promotable_cst) {
    if (params.cmap_impl == cmap_impl_type::std) {
      perform_tests<MultiLocalMapPromotable32CountSketch<RangeSize>,
                    make_shared_functor>(params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_impl == cmap_impl_type::boost) {
      perform_tests<MultiLocalFlatMapPromotable32CountSketch<RangeSize>,
                    make_shared_functor>(params);
#endif
    }
  } else if (params.sketch_impl == sketch_impl_type::fwht) {
    perform_tests<MultiLocalDense32FWHT<RangeSize>, make_shared_functor>(
        params);
  }
}

void choose_local_tests(const Parameters &params) {
  switch (params.range_size) {
    case 4:
      choose_local_tests_sized<4>(params);
      break;
    case 8:
      choose_local_tests_sized<8>(params);
      break;
    case 16:
      choose_local_tests_sized<16>(params);
      break;
    case 32:
      choose_local_tests_sized<32>(params);
      break;
    case 64:
      choose_local_tests_sized<64>(params);
      break;
    case 128:
      choose_local_tests_sized<128>(params);
      break;
    case 256:
      choose_local_tests_sized<256>(params);
      break;
    case 512:
      choose_local_tests_sized<512>(params);
      break;
    default:
      throw std::logic_error("Only accepts power-of-2 range size from 4-512");
  }
}

template <std::size_t RangeSize>
void do_all_local_tests_sized(const Parameters &params) {
  perform_tests<MultiLocalDense32CountSketch<RangeSize>, make_shared_functor>(
      params);
  perform_tests<MultiLocalMapSparse32CountSketch<RangeSize>,
                make_shared_functor>(params);
  perform_tests<MultiLocalMapPromotable32CountSketch<RangeSize>,
                make_shared_functor>(params);
#if __has_include(<boost/container/flat_map.hpp>)
  perform_tests<MultiLocalFlatMapSparse32CountSketch<RangeSize>,
                make_shared_functor>(params);
  perform_tests<MultiLocalFlatMapPromotable32CountSketch<RangeSize>,
                make_shared_functor>(params);
#endif
  perform_tests<MultiLocalDense32FWHT<RangeSize>, make_shared_functor>(params);
}

void do_all_local_tests(const Parameters &params) {
  switch (params.range_size) {
    case 4:
      do_all_local_tests_sized<4>(params);
      break;
    case 8:
      do_all_local_tests_sized<8>(params);
      break;
    case 16:
      do_all_local_tests_sized<16>(params);
      break;
    case 32:
      do_all_local_tests_sized<32>(params);
      break;
    case 64:
      do_all_local_tests_sized<64>(params);
      break;
    case 128:
      do_all_local_tests_sized<128>(params);
      break;
    case 256:
      do_all_local_tests_sized<256>(params);
      break;
    case 512:
      do_all_local_tests_sized<512>(params);
      break;
    default:
      throw std::logic_error("Only accepts power-of-2 range size from 4-512");
  }
}

int main(int argc, char **argv) {
  std::uint64_t    count(10000);
  std::uint64_t    range_size(16);
  std::uint64_t    seed(krowkee::hash::default_seed);
  std::size_t      compaction_threshold(10);
  std::size_t      promotion_threshold(8);
  sketch_impl_type sketch_impl(sketch_impl_type::cst);
  cmap_impl_type   cmap_impl(cmap_impl_type::std);
  bool             verbose(false);
  bool             do_all(argc == 1);

  Parameters params{count,
                    range_size,
                    compaction_threshold,
                    promotion_threshold,
                    seed,
                    sketch_impl,
                    cmap_impl,
                    verbose};

  parse_args(argc, argv, params);

  if (do_all == true) {
    do_all_local_tests(params);
  } else {
    choose_local_tests(params);
  }
  return 0;
}
