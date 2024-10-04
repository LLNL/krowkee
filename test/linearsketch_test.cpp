// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

// Klugy, but includes need to be in this order.

#include <check_archive.hpp>
#include <sketch_types.hpp>

#include <krowkee/hash/hash.hpp>
#include <krowkee/sketch/interface.hpp>
#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>
#include <krowkee/util/tests.hpp>

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

/**
 * Struct bundling the experiment parameters.
 */
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

/**
 * Verify that initialization and assignment (=) operators work as expected.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
struct init_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " constructors";
    return ss.str();
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr = make_ptr_t();
    for (std::uint64_t i(10); i < 100000; i *= 10) {
      sf_ptr_t sf_ptr(_make_ptr(i));
      ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);
      if (params.verbose == true) {
        std::cout << "input s = " << i << " produces " << ls.size() << "/"
                  << ls.range_size() << " registers of size " << ls.reg_size()
                  << " bytes each " << std::endl;
      }
    }
    {
      sf_ptr_t sf_ptr_1(_make_ptr(32));
      sf_ptr_t sf_ptr_2(_make_ptr(32));
      ls_t     ls_1(sf_ptr_1, params.compaction_threshold,
                    params.promotion_threshold);
      ls_t     ls_2(sf_ptr_2, params.compaction_threshold,
                    params.promotion_threshold);
      for (int i(0); i < 1000; i++) {
        ls_1.insert(i);
        ls_2.insert(i);
      }
      bool constructors_match = ls_1 == ls_2;
      if (constructors_match == false) {
        std::cout << "ls_1 : " << ls_1 << std::endl;
        std::cout << "ls_2 : " << ls_2 << std::endl;
      }
      CHECK_CONDITION(constructors_match == true,
                      "constructor/insert consistency");
    }
    {
      sf_ptr_t sf_ptr(_make_ptr(32));
      ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);
      for (int i(0); i < 1000; ls.insert(i++)) {
      }
      ls_t ls2(ls);
      bool copy_matches = ls == ls2;
      if (copy_matches == false) {
        std::cout << "ls : " << ls << std::endl;
        std::cout << "ls2 : " << ls2 << std::endl;
      }
      CHECK_CONDITION(copy_matches == true, "copy constructor");
    }
    {
      sf_ptr_t sf_ptr(_make_ptr(32));
      ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);
      for (int i(0); i < 1000; ls.insert(i++)) {
      }
      ls_t ls2          = ls;
      bool swap_matches = ls == ls2;
      if (swap_matches == false) {
        std::cout << "ls : " << ls << std::endl;
        std::cout << "ls2 : " << ls2 << std::endl;
      }
      CHECK_CONDITION(swap_matches, "copy-and-swap assignment");
    }
  }
};

/**
 * Verify that the given sketch functor produces a reasonable embedding.
 *
 * @note[BWP] TODO: implement some sort of statistical testing to verify that
 *     we are getting approximately correct shape preservation in embedded
 *     space.
 *
 * @note[BWP] `krowkee::CountSketchFunctor` templated with `krowkee::WangHash`
 *     produces weird and bad results here. We are unlikely to ever use
 *     `krowkee::WangHash`, but it might be worth figuring out what is wrong.
 *     It probably has something to do with the polarity hash, as it looks like
 *     the entries to the first 1/2 of registers are all -1, while the entries
 *     to the second 1/2 of registers are all +1.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
struct ingest_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  inline std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " ingest";
    return ss.str();
  }

  std::vector<std::vector<std::uint64_t>> get_uniform_inserts(
      const parameters_t &params) const {
    std::mt19937                                 gen(params.seed);
    std::uniform_int_distribution<std::uint64_t> dist(0,
                                                      params.domain_size - 1);

    std::vector<std::vector<std::uint64_t>> inserts(
        params.observation_count, std::vector<std::uint64_t>(params.count));

    for (std::int64_t i(0); i < params.observation_count; ++i) {
      for (std::int64_t j(0); j < params.count; ++j) {
        inserts[i][j] = dist(gen);
      }
    }
    return inserts;
  }

  template <typename T>
  double _l2_distance_sq(const std::vector<T> &lhs,
                         const std::vector<T> &rhs) const {
    assert(lhs.size() == rhs.size());
    double dist_sq(0);
    for (int i(0); i < lhs.size(); ++i) {
      dist_sq += std::pow(lhs[i] - rhs[i], 2);
    }
    return dist_sq;
  }

  void rel_mag_test(const sf_ptr_t &sf_ptr, const parameters_t &params) const {
    ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    std::uint64_t row_idx(17);
    for (std::uint64_t i(0); i < params.count; ls.insert(i++, row_idx)) {
    }
    ls.compactify();
    int    sum(accumulate(ls, 0.0));
    double rel_mag((double)sum / params.count);
    if (params.verbose == true) {
      std::cout << "\t" << ls << std::endl;
      std::cout << "\tregister sum (should be near zero): " << sum
                << ", relative magnitude: " << rel_mag << std::endl;
    }
    CHECK_CONDITION(rel_mag < 0.1, "register sum relative magnitude near zero");
  }

  template <typename T>
  void print_mat(const char *name, std::vector<std::vector<T>> &inserts,
                 const int nrows, const int ncols) const {
    std::cout << std::endl;
    std::cout << name << ":" << std::endl;
    for (int i(0); i < nrows; ++i) {
      std::cout << "(" << i << ")\t";
      for (int j(0); j < ncols; ++j) {
        std::cout << " " << inserts[i][j];
      }
      std::cout << std::endl;
    }
  }

  void print_mat(std::vector<ls_t> &sketches, const int nrows) const {
    std::cout << std::endl;
    std::cout << "sketches:" << std::endl;
    for (int i(0); i < nrows; ++i) {
      std::cout << "(" << i << ")\t" << sketches[i] << std::endl;
    }
  }

  std::vector<std::vector<int>> fill_observation_vector(
      const std::vector<std::vector<std::uint64_t>> &inserts,
      const parameters_t                            &params) const {
    std::vector<std::vector<int>> observations(
        params.observation_count, std::vector<int>(params.domain_size));
    for (int i(0); i < params.observation_count; ++i) {
      for (int j(0); j < params.count; ++j) {
        observations[i][inserts[i][j]]++;
      }
    }
    return observations;
  }

  std::vector<ls_t> fill_sketch_vector(
      const sf_ptr_t                                &sf_ptr,
      const std::vector<std::vector<std::uint64_t>> &inserts,
      const parameters_t                            &params) const {
    std::vector<ls_t> sketches(
        params.observation_count,
        ls_t(sf_ptr, params.compaction_threshold, params.promotion_threshold));
    for (int i(0); i < params.observation_count; ++i) {
      for (int j(0); j < params.count; ++j) {
        sketches[i].insert(inserts[i][j]);
      }
      sketches[i].compactify();
    }
    return sketches;
  }

  std::vector<std::vector<int>> fill_projection_vector(
      const std::vector<ls_t> &sketches, const parameters_t &params) const {
    std::vector<std::vector<int>> projections;
    for (int i(0); i < params.observation_count; ++i) {
      projections.push_back(sketches[i].register_vector());
    }
    return projections;
  }

  void lemma_check(const sf_ptr_t &sf_ptr, const parameters_t &params) const {
    ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);

    std::vector<std::vector<std::uint64_t>> inserts =
        get_uniform_inserts(params);

    std::vector<std::vector<int>> observations =
        fill_observation_vector(inserts, params);

    std::vector<ls_t> sketches = fill_sketch_vector(sf_ptr, inserts, params);

    std::vector<std::vector<int>> projections =
        fill_projection_vector(sketches, params);

    double epsilon =
        std::sqrt(8 * std::log(params.observation_count) / params.range_size);
    // compute inner products
    if (params.verbose) {
      print_mat("inserts", inserts, params.observation_count, params.count);
      print_mat("observations", observations, params.observation_count,
                params.domain_size);
      print_mat(sketches, params.observation_count);
      print_mat("projections", projections, params.observation_count,
                params.range_size);
      std::cout << std::endl;
      std::cout << "projected vectors:" << std::endl;
    }
    double success_rate(0.0);
    int    trials(0);
    for (int i(0); i < params.observation_count; ++i) {
      for (int j(0); j < params.observation_count; ++j) {
        if (i == j) {
          break;
        }
        ++trials;
        double ob_dist = _l2_distance_sq(observations[i], observations[j]);
        double sk_dist = _l2_distance_sq(projections[i], projections[j]);
        if (in_bounds(ob_dist, sk_dist, epsilon)) {
          success_rate += 1.0;
        }
        if (params.verbose) {
          std::cout << "\t(" << i << "," << j << ") ob " << ob_dist << ", sk "
                    << sk_dist
                    << " (in bounds: " << in_bounds(ob_dist, sk_dist, epsilon)
                    << ")" << std::endl;
        }
      }
    }
    success_rate /= trials;
    bool lemma_guarantee_success = success_rate > 0.5;
    CHECK_CONDITION(lemma_guarantee_success == true, "lemma guarantee (",
                    trials, " trials, ", success_rate,
                    " success rate, epsilon=", epsilon, ")");
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr{};
    sf_ptr_t   sf_ptr(_make_ptr(params.range_size, params.seed));
    rel_mag_test(sf_ptr, params);
    lemma_check(sf_ptr, params);
  }

  bool in_bounds(const double ob_dist, const double sk_dist,
                 const double epsilon) const {
    return (sk_dist < (1 + epsilon) * ob_dist) &&
           (sk_dist > (1 - epsilon) * ob_dist);
  }
};

template <typename SketchType>
void check_throws_bad_plus_equals(SketchType &lhs, const SketchType &rhs) {
  lhs += rhs;
}

template <typename SketchType>
void check_throws_bad_plus(SketchType &lhs, const SketchType &rhs) {
  SketchType ls = lhs += rhs;
}

/**
 * Verify that merge (+/+=) operators catch bad merges.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
struct bad_merge_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  inline std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " bad merges";
    return ss.str();
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr = make_ptr_t();
    sf_ptr_t   sf_ptr_1(_make_ptr(16));
    sf_ptr_t   sf_ptr_2(_make_ptr(32));
    sf_ptr_t   sf_ptr_3(_make_ptr(32, 22));
    ls_t       ls_1(sf_ptr_1, params.compaction_threshold,
                    params.promotion_threshold);
    ls_t       ls_2(sf_ptr_2, params.compaction_threshold,
                    params.promotion_threshold);
    ls_t       ls_3(sf_ptr_3, params.compaction_threshold,
                    params.promotion_threshold);
    CHECK_THROWS<std::invalid_argument>(
        check_throws_bad_plus_equals<SketchType>,
        "bad merge (+=) with different functor domains", ls_1, ls_2);
    CHECK_THROWS<std::invalid_argument>(
        check_throws_bad_plus_equals<SketchType>,
        "bad merge (+=) with different functor seeds", ls_2, ls_3);
    CHECK_THROWS<std::invalid_argument>(
        check_throws_bad_plus<SketchType>,
        "bad merge (+) with different functor domains", ls_1, ls_2);
    CHECK_THROWS<std::invalid_argument>(
        check_throws_bad_plus<SketchType>,
        "bad merge (+) with different functor seeds", ls_2, ls_3);
  }
};

/**
 * Verify that merge (+/+=) operators work as expected.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
struct good_merge_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  inline std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " good merges";
    return ss.str();
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr = make_ptr_t();
    sf_ptr_t   sf_ptr(_make_ptr(8));
    ls_t first(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t middle(sf_ptr, params.compaction_threshold,
                params.promotion_threshold);
    ls_t last(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t both(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t all(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    for (std::uint64_t i(0); i < 1000; ++i) {
      first.insert(i);
      both.insert(i);
      all.insert(i);
    }
    for (std::uint64_t i(1000); i < 2000; ++i) {
      middle.insert(i);
      both.insert(i);
      all.insert(i);
    }
    for (std::uint64_t i(1000); i < 2000; ++i) {
      last.insert(i);
      all.insert(i);
    }
    first.compactify();
    middle.compactify();
    last.compactify();
    both.compactify();
    all.compactify();
    ls_t bb = first + middle;
    bb.compactify();
    {
      bool merge_success = both == bb;
      CHECK_CONDITION(merge_success == true, "merge (+)");
    }
    {
      ls_t aa = first + middle + last;
      aa.compactify();
      bool multimerge_success = all == aa;
      CHECK_CONDITION(multimerge_success == true, "multi-merge (+, +)");
    }
    {
      first += middle;
      bool inplace_merge_success = both == first;
      CHECK_CONDITION(inplace_merge_success == true, "merge (+=)");
    }
  }
};

#if __has_include(<cereal/cereal.hpp>)
template <typename SketchType, template <typename> class MakePtrFunc>
struct serialize_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  inline std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " serialize";
    return ss.str();
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr{};
    sf_ptr_t   sf_ptr(_make_ptr(params.range_size, params.seed));

    CHECK_ALL_ARCHIVES(*sf_ptr, "sketch functor");

    ls_t ls(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    for (std::uint64_t i(0); i < params.count; ls.insert(i++)) {
    }
    ls.compactify();

    CHECK_ALL_ARCHIVES(ls.container(), "sketch container");
    CHECK_ALL_ARCHIVES(ls, "whole sketch object");
  }
};
#endif

/**
 * Verify that merge (+/+=) operators work as expected.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
struct promotion_check {
  typedef SketchType              ls_t;
  typedef typename ls_t::sf_t     sf_t;
  typedef typename ls_t::sf_ptr_t sf_ptr_t;
  typedef MakePtrFunc<sf_t>       make_ptr_t;

  inline std::string name() const {
    std::stringstream ss;
    ss << sf_t::name() << " promotion check";
    return ss.str();
  }

  void operator()(const parameters_t &params) const {
    make_ptr_t _make_ptr = make_ptr_t();
    sf_ptr_t   sf_ptr(_make_ptr(params.range_size, params.seed));
    ls_t s1(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t s2(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t d1(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t d2(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t d12(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    ls_t dall(sf_ptr, params.compaction_threshold, params.promotion_threshold);
    int  pt(params.promotion_threshold);
    for (std::uint64_t i(0); i < pt - 1; ++i) {
      s1.insert(i);
      d12.insert(i);
      d1.insert(i);
      d1.insert(i);
      d2.insert(i);
      dall.insert(i);
    }
    for (std::uint64_t i(pt); i < 2 * pt - 1; ++i) {
      s2.insert(i);
      d12.insert(i);
      d1.insert(i);
      d2.insert(i);
      d2.insert(i);
      dall.insert(i);
    }
    for (std::uint64_t i(2 * pt); i < params.count; ++i) {
      d1.insert(i);
      d2.insert(i);
      dall.insert(i);
    }
    s1.compactify();
    s2.compactify();
    d1.compactify();
    d2.compactify();
    d12.compactify();
    dall.compactify();
    {
      ls_t d1_ = s1 + dall;
      d1_.compactify();
      bool sp_merge_success = d1_ == d1;
      CHECK_CONDITION(sp_merge_success == true, "merge (+) sparse/dense");
    }
    {
      ls_t d1_ = dall + s1;
      d1_.compactify();
      bool sp_merge_success = d1_ == d1;
      CHECK_CONDITION(sp_merge_success == true, "merge (+) dense/sparse");
    }
    {
      ls_t dall12 = dall + s1 + s2;
      ls_t d12_   = dall + d12;
      dall12.compactify();
      d12_.compactify();
      bool sp_multimerge_success = dall12 == d12_;
      CHECK_CONDITION(sp_multimerge_success == true,
                      "multi-merge (+) dense/sparse/sparse");
    }
    {
      ls_t s11 = s1 + s1;
      s11.compactify();
      bool sss_merge_success =
          accumulate(s11, 0.0) == 2 * accumulate(s1, 0.0) &&
          s11.is_sparse() == true;
      CHECK_CONDITION(sss_merge_success == true,
                      "merge (+) sparse/sparse (sparse)");
    }
    {
      ls_t s12 = s1 + s2;
      s12.compactify();
      bool ssd_merge_success = s12 == d12;
      CHECK_CONDITION(ssd_merge_success == true,
                      "merge (+) sparse/sparse (dense)");
    }
    {
      s1 += dall;
      s1.compactify();
      bool ssd_inplace_merge_success = s1 == d1;
      CHECK_CONDITION(ssd_inplace_merge_success == true,
                      "merge (+=) sparse/dense (dense)");
    }
    {
      dall += s2;
      dall.compactify();
      bool dsd_inplace_merge_success = dall == d2;
      CHECK_CONDITION(dsd_inplace_merge_success == true,
                      "merge (+=) dense/sparse (dense)");
    }
  }
};

/**
 * Execute the batter of tests for the given sketch functor.
 */
template <typename SketchType, template <typename> class MakePtrFunc>
void perform_tests(const parameters_t &params) {
  typedef SketchType          ls_t;
  typedef typename ls_t::sf_t sf_t;

  MakePtrFunc<std::int32_t> mpf;

  print_line();
  print_line();
  std::cout << "Testing " << ls_t::full_name() << std::endl;
  std::cout << "\tUsing " << mpf.name() << std::endl;
  print_line();
  print_line();

  std::cout << std::endl << std::endl;

  do_test<init_check<ls_t, MakePtrFunc>>(params);
  do_test<ingest_check<ls_t, MakePtrFunc>>(params);
  do_test<bad_merge_check<ls_t, MakePtrFunc>>(params);
  do_test<good_merge_check<ls_t, MakePtrFunc>>(params);
#if __has_include(<cereal/cereal.hpp>)
  do_test<serialize_check<ls_t, MakePtrFunc>>(params);
#endif
  if (params.sketch_type == sketch_type_t::promotable_cst &&
      params.promotion_threshold < params.range_size) {
    do_test<promotion_check<ls_t, MakePtrFunc>>(params);
  }
}

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

void choose_tests(const parameters_t &params) {
  if (params.sketch_type == sketch_type_t::cst) {
    perform_tests<Dense32CountSketch, make_ptr_functor_t>(params);
  } else if (params.sketch_type == sketch_type_t::sparse_cst) {
    if (params.cmap_type == cmap_type_t::std) {
      perform_tests<MapSparse32CountSketch, make_ptr_functor_t>(params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_type == cmap_type_t::boost) {
      perform_tests<FlatMapSparse32CountSketch, make_ptr_functor_t>(params);
#endif
    }
  } else if (params.sketch_type == sketch_type_t::promotable_cst) {
    if (params.cmap_type == cmap_type_t::std) {
      perform_tests<MapPromotable32CountSketch, make_ptr_functor_t>(params);
#if __has_include(<boost/container/flat_map.hpp>)
    } else if (params.cmap_type == cmap_type_t::boost) {
      perform_tests<FlatMapPromotable32CountSketch, make_ptr_functor_t>(params);
#endif
    }
  } else if (params.sketch_type == sketch_type_t::fwht) {
    perform_tests<Dense32FWHT, make_ptr_functor_t>(params);
  }
}

void do_all_tests(const parameters_t &params) {
  perform_tests<Dense32CountSketch, make_ptr_functor_t>(params);
  perform_tests<MapSparse32CountSketch, make_ptr_functor_t>(params);
  perform_tests<MapPromotable32CountSketch, make_ptr_functor_t>(params);
#if __has_include(<boost/container/flat_map.hpp>)
  perform_tests<FlatMapSparse32CountSketch, make_ptr_functor_t>(params);
  perform_tests<FlatMapPromotable32CountSketch, make_ptr_functor_t>(params);
#endif
  // perform_tests<Dense32FWHT, make_ptr_functor_t>(params);
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

  if (do_all == true) {
    do_all_tests(params);
  } else {
    choose_tests(params);
  }
  return 0;
}