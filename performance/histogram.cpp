// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <parameters.hpp>
#include <sampling.hpp>
#include <sketch_interface.hpp>
#include <timing.hpp>
#include <types.hpp>
#include <utils.hpp>

template <typename ValueType>
struct vector_hist_t : public vector_t<ValueType> {
  vector_hist_t(const parameters_t &params) : vector_t<ValueType>(params) {}

  template <typename KeyType>
  void insert(const KeyType idx) {
    this->check_bounds(idx);
    this->_hist[idx]++;
  }
};

template <typename KeyType, typename ValueType>
struct map_hist_t : map_t<KeyType, ValueType> {
  map_hist_t(const parameters_t &params) : map_t<KeyType, ValueType>(params) {}

  void insert(const KeyType idx) {
    this->check_bounds(idx);
    this->_hist[idx]++;
  }
};

template <typename KeyType>
struct set_hist_t : set_t<KeyType> {
  set_hist_t(const parameters_t &params) : set_t<KeyType>(params) {}

  void insert(const KeyType idx) {
    this->check_bounds(idx);
    this->_hist.insert(idx);
  }
};

void benchmark(const parameters_t &params) {
  using sample_t = std::uint64_t;
  std::vector<sample_t> samples(make_samples<sample_t>(params));

#if __has_include(<boost/container/flat_map.hpp>)
  auto sk_profiles = profile_sketches<sample_t, dense32_hist_cs_t<sample_t>,
                                      map_sparse32_hist_cs_t<sample_t>,
                                      map_promotable32_hist_cs_t<sample_t>,
                                      flatmap_sparse32_hist_cs_t<sample_t>,
                                      flatmap_promotable32_hist_cs_t<sample_t>>(
      samples, params);
#else
  auto sk_profiles =
      profile_sketches<sample_t, dense32_hist_cs_t<sample_t>,
                       map_sparse32_hist_cs_t<sample_t>,
                       map_promotable32_hist_cs_t<sample_t>>(samples, params);
#endif

  auto hist_profiles =
      profile_histograms<sample_t, vector_hist_t<sample_t>,
                         set_hist_t<sample_t>, map_hist_t<sample_t, sample_t>>(
          samples, params);
  print_profiles(sk_profiles, hist_profiles);
}

int main(int argc, char **argv) {
  parameters_t params = parse_args(argc, argv);

  benchmark(params);
  return 0;
}