// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <parameters.hpp>
#include <sketch_interface.hpp>
#include <timing.hpp>
#include <utils.hpp>

template <typename SampleType, typename... HistTypes>
void benchmark(std::vector<SampleType> &samples, const parameters_t &params) {
  using sample_t = SampleType;
#if __has_include(<boost/container/flat_map.hpp>)
  auto sk_profiles =
      profile_sketches<sample_t, Dense32CountSketch, MapSparse32CountSketch,
                       MapPromotable32CountSketch, FlatMapSparse32CountSketch,
                       FlatMapPromotable32CountSketch>(samples, params);
#else
  auto sk_profiles =
      profile_sketches<sample_t, Dense32CountSketch, MapSparse32CountSketch,
                       MapPromotable32CountSketch>(samples, params);
#endif

  auto hist_profiles =
      profile_histograms<sample_t, HistTypes...>(samples, params);
  print_profiles(sk_profiles, hist_profiles);
}