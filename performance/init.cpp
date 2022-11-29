// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <parameters.hpp>
#include <sketch_interface.hpp>
#include <timing.hpp>
#include <types.hpp>
#include <utils.hpp>

template <typename ValueType>
using vector_init_t = vector_t<ValueType>;

template <typename KeyType, typename ValueType>
struct map_init_t : map_t<KeyType, ValueType> {
  map_init_t(const parameters_t &params) : map_t<KeyType, ValueType>(params) {
    for (int i(0); i < params.domain_size; ++i) {
      this->_map[i] = 0;
    }
  }
};

template <typename KeyType>
struct set_init_t : set_t<KeyType> {
  set_init_t(const parameters_t &params) : set_t<KeyType>(params) {
    for (int i(0); i < params.domain_size; ++i) {
      this->_set.insert(i);
    }
  }
};

void benchmark(const parameters_t &params) {
  using sample_t = std::uint64_t;
#if __has_include(<boost/container/flat_map.hpp>)
  auto sk_profiles =
      profile_sketch_init<map_sparse32_hist_cs_t<sample_t>,
                          dense32_hist_cs_t<sample_t>,
                          map_promotable32_hist_cs_t<sample_t>,
                          flatmap_sparse32_hist_cs_t<sample_t>,
                          flatmap_promotable32_hist_cs_t<sample_t>>(params);
#else
  auto sk_profiles =
      profile_sketch_init<dense32_hist_cs_t<sample_t>,
                          map_sparse32_hist_cs_t<sample_t>,
                          map_promotable32_hist_cs_t<sample_t>>(params);
#endif

  auto hist_profiles =
      profile_container_init<vector_init_t<sample_t>, set_init_t<sample_t>,
                             map_init_t<sample_t, sample_t>>(params);
  print_profiles(sk_profiles, hist_profiles);
}

int main(int argc, char **argv) {
  parameters_t params = parse_args(argc, argv);

  benchmark(params);
  return 0;
}