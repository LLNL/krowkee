// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <random>

std::vector<std::uint64_t> make_samples(const parameters_t &params) {
  std::mt19937                                 gen(params.seed);
  std::uniform_int_distribution<std::uint64_t> dist(0, params.domain_size - 1);

  std::vector<std::uint64_t> samples(params.count);

  for (std::int64_t i(0); i < params.count; ++i) {
    samples[i] = dist(gen);
  }
  return samples;
}
