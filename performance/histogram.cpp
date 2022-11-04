// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <benchmark.hpp>
#include <sampling.hpp>
#include <types.hpp>

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
  std::vector<sample_t> samples(make_samples(params));

  benchmark<sample_t, vector_hist_t<sample_t>, set_hist_t<sample_t>,
            map_hist_t<sample_t, std::uint64_t>>(samples, params);
}

int main(int argc, char **argv) {
  parameters_t params = parse_args(argc, argv);

  benchmark(params);
  return 0;
}