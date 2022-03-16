// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_UTIL_MPITESTS_HPP
#define _KROWKEE_UTIL_MPITESTS_HPP

#include <krowkee/util/ygm_tests.hpp>

#include <ygm/comm.hpp>

inline void chirp0(ygm::comm &world) {
  if (world.rank0()) {
    std::cout << "gets here" << std::endl;
  }
}

template <typename FuncType, typename... Args>
void do_mpi_test(ygm::comm &world, Args &&...args) {
  FuncType func;
  if (world.rank0()) {
    print_line();
    std::cout << func.name() << ":" << std::endl;
    print_line();
  }
  auto start(Clock::now());
  func(world, args...);
  world.barrier();
  auto end(Clock::now());
  auto ns(std::chrono::duration_cast<ns_t>(end - start).count());
  if (world.rank0()) {
    print_line();
    std::cout << "\tTest time: " << ((double)ns / 1e9) << "s" << std::endl;
    std::cout << std::endl << std::endl;
  }
}

#endif