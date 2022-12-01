// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <iostream>

void print_profiles(
    const std::vector<std::pair<std::string, double>> &sk_profiles,
    const std::vector<std::pair<std::string, double>> &hist_profiles) {
  for (const auto sk_pair : sk_profiles) {
    std::cout << sk_pair.first << " " << sk_pair.second << "s" << std::endl;
    for (const auto hist_pair : hist_profiles) {
      std::cout << "\tvs " << hist_pair.first << " " << hist_pair.second
                << "s (" << (sk_pair.second / hist_pair.second) << "x slowdown)"
                << std::endl;
    }
  }
}