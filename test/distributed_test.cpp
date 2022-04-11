// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee/sketch/interface.hpp>
#include <krowkee/stream/interface.hpp>

#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/mpitests.hpp>
#include <krowkee/util/sketch_types.hpp>

#include <ygm/comm.hpp>
#include <ygm/container/map.hpp>

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

template <typename Key, typename Value>
using ygm_map = ygm::container::map<Key, Value>;

using CountingDistributedDense32CountSketch =
    krowkee::stream::CountingDistributedCountSketch<
        krowkee::sketch::Dense, std::uint64_t, std::int32_t>;

using CountingDistributedMapSparse32CountSketch =
    krowkee::stream::CountingDistributedCountSketch<
        krowkee::sketch::MapSparse32, std::uint64_t, std::int32_t>;

using CountingDistributedMapPromotable32CountSketch =
    krowkee::stream::CountingDistributedCountSketch<
        krowkee::sketch::MapPromotable32, std::uint64_t, std::int32_t>;

#if __has_include(<boost/container/flat_map.hpp>)
using CountingDistributedFlatMapSparse32CountSketch =
    krowkee::stream::CountingDistributedCountSketch<
        krowkee::sketch::FlatMapSparse32, std::uint64_t, std::int32_t>;

using CountingDistributedFlatMapPromotable32CountSketch =
    krowkee::stream::CountingDistributedCountSketch<
        krowkee::sketch::FlatMapPromotable32, std::uint64_t, std::int32_t>;
#endif

using CountingDistributedDense32FWHT =
    krowkee::stream::CountingDistributedFWHT<std::uint64_t, std::int32_t>;

/**
 * Struct bundling the experiment parameters.
 */
struct parameters_t {
  std::uint64_t count;
  std::uint64_t range_size;
  std::size_t   compaction_threshold;
  std::size_t   promotion_threshold;
  std::uint64_t seed;
  sketch_type_t sketch_type;
  cmap_type_t   cmap_type;
  bool          verbose;
};

struct merge_test_t {
  template <typename M, typename T>
  void set_up_map(ygm::comm &world, M &dmap, const parameters_t &params,
                  const T &obj1, const T &obj2, const T &obj3) {
    if (world.rank0()) {
      dmap.async_insert(1, obj1);
    }
    if (world.rank() == 1) {
      dmap.async_insert(2, obj2);
    }
    world.barrier();
  }

  template <typename M, typename T>
  void operator()(ygm::comm &world, M &dmap, const parameters_t &params,
                  const std::string &qualifier, const T &obj1, const T &obj2,
                  const T &obj3) const {
    auto first_visit_lambda = [](auto pmap, auto kv_pair, bool verbose,
                                 T expected_merge_val, std::string msg,
                                 int second_key) {
      if (verbose == true) {
        std::cout << "First visit:"
                  << "\nI am rank " << pmap->comm().rank()
                  << " and a rank is asking to forward the sketch"
                  << "\n\tKey: " << kv_pair.first << "\n\tto destination "
                  << second_key << std::endl;
      }

      auto second_visit_lambda = [](auto pmap, auto kv_pair, bool verbose,
                                    T expected_merge_val, std::string msg,
                                    T fwd_val) {
        T    merge_val = kv_pair.second + fwd_val;
        bool match     = merge_val == expected_merge_val;
        if (verbose == true) {
          std::cout << "Second visit:"
                    << "\nI am rank " << pmap->comm().rank()
                    << " and a rank is asking to merge"
                    << "\n\tKey: " << kv_pair.first
                    << "\n\tVal: " << kv_pair.second
                    << "\n\twith forwarded value: " << fwd_val
                    << "\n\tobtained merge: " << expected_merge_val
                    << "\n\texpected merge: " << expected_merge_val
                    << "\n\tValues "
                    << ((match == true) ? "match!" : "don't match!")
                    << std::endl;
        }
        auto third_visit_lambda = [](auto pcomm, bool verbose, std::string msg,
                                     bool match) {
          if (verbose == true) {
            std::cout << "Third visit:"
                      << "\nI am rank " << pcomm->rank()
                      << " and a rank informed me that the merged values "
                      << ((match == true) ? "matched" : "did not match")
                      << std::endl;
          }
          CHECK_CONDITION(match == true, msg);
        };
        pmap->comm().async(0, third_visit_lambda, verbose, msg, match);
      };

      pmap->async_visit(second_key, second_visit_lambda, verbose,
                        expected_merge_val, msg, kv_pair.second);
    };
    if (world.rank() == 3) {
      std::stringstream ss;
      ss << "distributed merge";
      if (qualifier.length() > 0) {
        ss << " " << qualifier;
      }
      dmap.async_visit(1, first_visit_lambda, params.verbose, obj3, ss.str(),
                       2);
    }
  }
};

struct equality_test_t {
  template <typename M, typename T>
  void set_up_map(ygm::comm &world, M &dmap, const parameters_t &params,
                  const T &obj1, const T &obj2) {
    if (world.rank0()) {
      dmap.async_insert(1, obj1);
    }
    if (world.rank() == 1) {
      dmap.async_insert(2, obj1);
    }
    if (world.rank() == 2) {
      dmap.async_insert(3, obj2);
    }
    world.barrier();
  }

  template <typename M, typename T>
  void operator()(ygm::comm &world, M &dmap, const parameters_t &params,
                  const std::string &qualifier, const T &obj1,
                  const T &obj2) const {
    auto first_visit_lambda = [](auto pmap, auto kv_pair, bool verbose,
                                 bool expected_match, std::string msg,
                                 int second_key) {
      if (verbose == true) {
        std::cout << "First visit:"
                  << "\nI am rank " << pmap->comm().rank()
                  << " and a rank is asking for a lookup of key "
                  << kv_pair.first << " to be checked against another key "
                  << second_key << "\n\tVal: " << kv_pair.second << std::endl;
      }

      auto second_visit_lambda = [](auto pmap, auto kv_pair, bool verbose,
                                    bool expected_match, std::string msg,
                                    T fwd_val) {
        bool match = kv_pair.second == fwd_val;
        if (verbose == true) {
          std::cout << "Second visit:"
                    << "\nI am rank " << pmap->comm().rank()
                    << " and a rank is asking to check key " << kv_pair.first
                    << "\n\tVal: " << kv_pair.second
                    << "\n\tAgainst forwarded value: " << fwd_val
                    << "\n\tValues "
                    << ((match == true) ? "match!" : "don't match!")
                    << std::endl;
        }
        auto third_visit_lambda = [](auto pcomm, bool verbose,
                                     bool expected_match, std::string msg,
                                     bool match) {
          if (verbose == true) {
            std::cout << "Third visit:"
                      << "\nI am rank " << pcomm->rank()
                      << " and a rank informed me that the values "
                      << ((match == true) ? "matched" : "did not match")
                      << std::endl;
          }
          CHECK_CONDITION(match == expected_match, msg);
        };
        pmap->comm().async(0, third_visit_lambda, verbose, expected_match, msg,
                           match);
      };

      pmap->async_visit(second_key, second_visit_lambda, verbose,
                        expected_match, msg, kv_pair.second);
    };
    if (world.rank() == 3) {
      std::stringstream ss;
      ss << T::name() << " distributed ==";
      if (qualifier.length() > 0) {
        ss << " " << qualifier;
      }
      dmap.async_visit(1, first_visit_lambda, params.verbose, true, ss.str(),
                       2);
    }
    world.barrier();
    if (world.rank() == 3) {
      std::stringstream ss;
      ss << T::name() << " distributed !=";
      if (qualifier.length() > 0) {
        ss << " " << qualifier;
      }
      dmap.async_visit(1, first_visit_lambda, params.verbose, false, ss.str(),
                       3);
    }
  }
};

template <typename T>
struct cross_map_equality_test_t {
  template <typename M>
  void operator()(ygm::comm &world, M &dmap1, M &dmap2,
                  const parameters_t &params,
                  const std::string  &qualifier) const {
    auto first_visit_lambda = [](auto pmap1, auto kv_pair, auto pmap2,
                                 bool verbose, bool expected_match,
                                 std::string msg) {
      if (verbose == true) {
        std::cout << "First visit:"
                  << "\nI am rank " << pmap1->comm().rank()
                  << " and a rank is asking for a lookup of key "
                  << kv_pair.first << " to be checked against the same key "
                  << kv_pair.first << "\n\tVal: " << kv_pair.second
                  << std::endl;
      }

      auto second_visit_lambda = [](auto pmap, auto kv_pair, bool verbose,
                                    bool expected_match, std::string msg,
                                    T fwd_val) {
        bool match = kv_pair.second == fwd_val;
        if (verbose == true) {
          std::cout << "Second visit:"
                    << "\nI am rank " << pmap->comm().rank()
                    << " and a rank is asking to check key " << kv_pair.first
                    << "\n\tVal: " << kv_pair.second
                    << "\n\tAgainst forwarded value: " << fwd_val
                    << "\n\tValues "
                    << ((match == true) ? "match!" : "don't match!")
                    << std::endl;
        }
        auto third_visit_lambda = [](auto pcomm, bool verbose,
                                     bool expected_match, std::string msg,
                                     bool match) {
          if (verbose == true) {
            std::cout << "Third visit:"
                      << "\nI am rank " << pcomm->rank()
                      << " and a rank informed me that the values "
                      << ((match == true) ? "matched" : "did not match")
                      << std::endl;
          }
          CHECK_CONDITION(match == expected_match, msg);
        };
        pmap->comm().async(0, third_visit_lambda, verbose, expected_match, msg,
                           match);
      };

      pmap2->async_visit(kv_pair.first, second_visit_lambda, verbose,
                         expected_match, msg, kv_pair.second);
    };
    if (world.rank() == 3) {
      for (int i(1); i < 4; ++i) {
        std::stringstream ss;
        ss << T::name() << " cross-map distributed ==";
        if (qualifier.length() > 0) {
          ss << " " << qualifier;
        }
        ss << " (" << i << " of 3)";
        dmap1.async_visit(i, first_visit_lambda, dmap2.get_ygm_ptr(),
                          params.verbose, true, ss.str());
      }
    }
    world.barrier();
  }
};

template <typename DistributedType>
struct communicable_check {
  typedef DistributedType              dsk_t;
  typedef typename dsk_t::sk_t         sk_t;
  typedef typename sk_t::container_t   con_t;
  typedef typename sk_t::sf_t          sf_t;
  typedef typename sk_t::sf_ptr_t      sf_ptr_t;
  typedef make_ygm_ptr_functor_t<sf_t> make_ygm_ptr_t;

  std::string name() const {
    std::stringstream ss;
    ss << sk_t::name() << " communicable parts test";
    return ss.str();
  }

  // This is inelegant but I'm wasted enough time already...
  template <typename Func, typename T, typename... Args>
  void make_ygm_map_and_test(ygm::comm &world, const parameters_t &params,
                             const T &obj1, const Args &...args) const {
    Func                        func;
    ygm::container::map<int, T> dmap(world);
    func.set_up_map(world, dmap, params, obj1, args...);
    func(world, dmap, params, "", obj1, args...);
  }

  void operator()(ygm::comm &world, const parameters_t &params) const {
    make_ygm_ptr_t make_ygm_ptr = make_ygm_ptr_t();
    sf_ptr_t       sf_ptr(make_ygm_ptr(params.range_size, params.seed));
    sf_ptr_t       sf_ptr2(make_ygm_ptr(params.range_size, params.seed + 5));

    make_ygm_map_and_test<equality_test_t>(world, params, *sf_ptr, *sf_ptr2);

    sk_t sk1(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    sk_t sk2(sf_ptr, params.compaction_threshold, params.promotion_threshold);

    for (std::uint64_t i(0); i < params.count; ++i) {
      sk1.insert(i);
      sk2.insert(i + params.count);
    }
    sk1.compactify();
    sk2.compactify();

    make_ygm_map_and_test<equality_test_t>(world, params, sk1.container(),
                                           sk2.container());

    make_ygm_map_and_test<equality_test_t>(world, params, sk1, sk2);

    sk_t sk3 = sk1 + sk2;

    make_ygm_map_and_test<merge_test_t>(world, params, sk1, sk2, sk3);
  }
};

template <typename DistributedType>
struct ingest_check {
  typedef DistributedType              dsk_t;
  typedef typename dsk_t::data_t       data_t;
  typedef typename data_t::sk_t        sk_t;
  typedef typename sk_t::container_t   con_t;
  typedef typename sk_t::sf_t          sf_t;
  typedef typename sk_t::sf_ptr_t      sf_ptr_t;
  typedef make_ygm_ptr_functor_t<sf_t> make_ygm_ptr_t;

  std::string name() const {
    std::stringstream ss;
    ss << dsk_t::name() << " ingest test";
    return ss.str();
  }

  template <typename Func, typename... Args>
  void make_Distributed_and_test(ygm::comm &world, const sf_ptr_t &sf_ptr,
                                 const parameters_t &params,
                                 const std::string  &qualifier,
                                 const Args &...args) const {
    Func  func;
    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);
    func.set_up_map(world, dsk, params, args...);
    func(world, dsk.ygm_map(), params, qualifier, args...);
  }

  void copy_check(ygm::comm &world, const sf_ptr_t &sf_ptr,
                  const parameters_t &params, const data_t &d1,
                  const data_t &d2, const data_t &d3) const {
    cross_map_equality_test_t<data_t> cross_func;
    equality_test_t                   func;
    // equality_test_t func;

    dsk_t dsk1(world, sf_ptr, params.compaction_threshold,
               params.promotion_threshold);

    if (world.rank0()) {
      dsk1.async_insert(1, d3);
    }
    if (world.rank() == 1) {
      dsk1.async_insert(2, d2);
    }
    if (world.rank() == 2) {
      dsk1.async_insert(3, d1);
    }
    world.barrier();

    dsk_t dsk2(dsk1);
    if (world.rank0()) {
      CHECK_CONDITION(dsk1.params_agree(dsk2), "parameter agreement (copy)");
    }
    if (world.rank0()) {
      CHECK_CONDITION(
          dsk1.ygm_map().default_value() == dsk2.ygm_map().default_value(),
          "default value agreement (copy)");
    }

    std::size_t dsk1_size(dsk1.size());
    std::size_t dsk2_size(dsk2.size());
    if (world.rank0()) {
      CHECK_CONDITION(dsk1_size == dsk2_size, "size agreement (copy)");
    }

    cross_func(world, dsk1.ygm_map(), dsk2.ygm_map(), params,
               "(copy agreement)");

    if (world.rank0()) {
      dsk1.async_merge(dsk2, 3, 2);
    }
    world.barrier();

    func(world, dsk1.ygm_map(), params, "(cross-dsk merge agreement)", d1, d2);
  }

  void insert_once_and_check(ygm::comm &world, const sf_ptr_t &sf_ptr,
                             const parameters_t &params,
                             const data_t       &d1) const {
    equality_test_t func;

    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);

    data_t data(d1);

    data.update(params.count + 1);

    if (world.rank0()) {
      dsk.async_insert(1, d1);
      dsk.async_update(1, params.count + 1);
    }
    if (world.rank() == 1) {
      dsk.async_insert(2, data);
    }
    if (world.rank() == 2) {
      dsk.async_insert(3, d1);
    }
    world.barrier();
    func(world, dsk.ygm_map(), params, "(insert once and check)", d1, data);
  }

  void insert_equality(ygm::comm &world, const sf_ptr_t &sf_ptr,
                       const parameters_t &params, const data_t &d1,
                       const data_t &d2) const {
    equality_test_t func;

    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);

    // data_t d3{sf_ptr, params.compaction_threshold,
    // params.promotion_threshold};
    if (world.rank0()) {
      for (int i(0); i < params.count; dsk.async_update(1, i++)) {
      }
      dsk.compactify(1);
    }
    if (world.rank() == 1) {
      dsk.async_insert(2, d1);
    }
    if (world.rank() == 2) {
      dsk.async_insert(3, d2);
    }
    world.barrier();
    func(world, dsk.ygm_map(), params, "(distributed insert)", d1, d2);
  }

  void distributed_merge(ygm::comm &world, const sf_ptr_t &sf_ptr,
                         const parameters_t &params, const data_t &d1,
                         const data_t &d2, const data_t &d3) const {
    equality_test_t func;

    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);

    // data_t d3{sf_ptr, params.compaction_threshold,
    // params.promotion_threshold};
    if (world.rank0()) {
      dsk.async_insert(1, d1);
      dsk.async_insert(0, d2);
    }
    world.barrier();
    if (world.rank() == 1) {
      dsk.async_merge(0, 1);
      dsk.async_insert(2, d3);
    }
    if (world.rank() == 2) {
      dsk.async_insert(3, d2);
    }
    world.barrier();
    func(world, dsk.ygm_map(), params, "(distributed merge)", d1, d2);
    world.barrier();

    dsk.for_all([](auto kv_pair) {
      std::cout << "I am key " << kv_pair.first << " holding sketch of size "
                << kv_pair.second.sk.size() << std::endl;
    });
  }

  void default_value(ygm::comm &world, const sf_ptr_t &sf_ptr,
                     const parameters_t &params) const {
    equality_test_t func;

    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);

    data_t d{sf_ptr, params.compaction_threshold, params.promotion_threshold};
    bool   default_correct = dsk.ygm_map().default_value() == d;
    if (world.rank0()) {
      CHECK_CONDITION(default_correct == true, "map default initialization");
    }
  }

  void operator()(ygm::comm &world, const parameters_t &params) const {
    make_ygm_ptr_t make_ygm_ptr = make_ygm_ptr_t();
    sf_ptr_t       sf_ptr(make_ygm_ptr(params.range_size, params.seed));

    data_t d1(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    data_t d2(sf_ptr, params.compaction_threshold, params.promotion_threshold);

    for (std::uint64_t i(0); i < params.count; ++i) {
      d1.update(i);
      d2.update(i + params.count);
    }
    d1.compactify();
    d2.compactify();

    make_Distributed_and_test<equality_test_t>(world, sf_ptr, params,
                                               "(equal size)", d1, d2);

    data_t d3 = d1 + d2;

    make_Distributed_and_test<merge_test_t>(world, sf_ptr, params, "", d1, d2,
                                            d3);

    copy_check(world, sf_ptr, params, d1, d2, d3);

    insert_once_and_check(world, sf_ptr, params, d1);

    default_value(world, sf_ptr, params);

    insert_equality(world, sf_ptr, params, d1, d2);

    distributed_merge(world, sf_ptr, params, d1, d2, d3);
  }
};

void print_help(char *exe_name) {
  std::cout << "\nusage:  " << exe_name << "\n"
            << "\t-c, --count <int>              - number of insertions\n"
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

template <typename DistributedType>
void perform_tests(ygm::comm &world, const parameters_t &params) {
  typedef DistributedType      dsk_t;
  typedef typename dsk_t::sk_t sk_t;
  typedef typename sk_t::sf_t  sf_t;

  if (world.rank0()) {
    print_line();
    print_line();
    std::cout << "Testing " << dsk_t::full_name() << std::endl;
    print_line();
    print_line();

    std::cout << std::endl << std::endl;
  }
  do_mpi_test<communicable_check<sk_t>>(world, params);
  do_mpi_test<ingest_check<dsk_t>>(world, params);
}

void choose_test(ygm::comm &world, const parameters_t &params) {
  if (params.sketch_type == sketch_type_t::cst) {
    perform_tests<CountingDistributedDense32CountSketch>(world, params);
  } else if (params.sketch_type == sketch_type_t::sparse_cst) {
    if (params.cmap_type == cmap_type_t::std) {
      perform_tests<CountingDistributedMapSparse32CountSketch>(world, params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_type == cmap_type_t::boost) {
      perform_tests<CountingDistributedFlatMapSparse32CountSketch>(world,
                                                                   params);
#endif
    }
  } else if (params.sketch_type == sketch_type_t::promotable_cst) {
    if (params.cmap_type == cmap_type_t::std) {
      perform_tests<CountingDistributedMapPromotable32CountSketch>(world,
                                                                   params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_type == cmap_type_t::boost) {
      perform_tests<CountingDistributedFlatMapPromotable32CountSketch>(world,
                                                                       params);
#endif
    }
  } else if (params.sketch_type == sketch_type_t::fwht) {
    perform_tests<CountingDistributedDense32FWHT>(world, params);
  }
}

void do_all_tests(ygm::comm &world, const parameters_t &params) {
  perform_tests<CountingDistributedDense32CountSketch>(world, params);
  perform_tests<CountingDistributedMapSparse32CountSketch>(world, params);
  perform_tests<CountingDistributedMapPromotable32CountSketch>(world, params);
#if __has_include(<boost/container/flat_map.hpp>)
  perform_tests<CountingDistributedFlatMapSparse32CountSketch>(world, params);
  perform_tests<CountingDistributedFlatMapPromotable32CountSketch>(world,
                                                                   params);
#endif
  perform_tests<CountingDistributedDense32FWHT>(world, params);
}

int main(int argc, char **argv) {
  std::uint64_t count(10000);
  std::uint64_t range_size(16);
  std::uint64_t seed(krowkee::hash::default_seed);
  std::size_t   compaction_threshold(10);
  std::size_t   promotion_threshold(8);
  sketch_type_t sketch_type(sketch_type_t::cst);
  cmap_type_t   cmap_type(cmap_type_t::std);
  bool          verbose(false);
  bool          do_all(argc == 1);

  parameters_t params{count,
                      range_size,
                      compaction_threshold,
                      promotion_threshold,
                      seed,
                      sketch_type,
                      cmap_type,
                      verbose};

  ygm::comm world(&argc, &argv);

  if (do_all == true) {
    do_all_tests(world, params);
  } else {
    parse_args(argc, argv, params);
    choose_test(world, params);
  }
}