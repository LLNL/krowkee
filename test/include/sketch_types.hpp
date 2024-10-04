// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/sketch/interface.hpp>
#include <krowkee/util/tests.hpp>

#if __has_include(<ygm/comm.hpp>)
#include <ygm/detail/ygm_ptr.hpp>

template <typename T>
using ptr_type = ygm::ygm_ptr<T>;

template <typename T>
ygm::ygm_ptr<T> _make_ygm_ptr(T &t) {
  return ygm::ygm_ptr<T>(&t);
}

/**
 * Functor wrapping _make_ygm_ptr
 */
template <typename T>
struct make_ygm_ptr_functor_t {
  typedef ygm::ygm_ptr<T> ptr_t;

  make_ygm_ptr_functor_t() {}

  template <typename... Args>
  ptr_t operator()(Args... args) {
    sptrs.push_back(std::make_unique<T>(args...));
    return _make_ygm_ptr<T>(*(sptrs.back()));
  }

  static constexpr std::string name() { return "ygm::ygm_ptr"; }

 private:
  std::vector<std::unique_ptr<T>> sptrs;
};

template <typename T>
using make_ptr_functor_t = make_ygm_ptr_functor_t<T>;
#else

template <typename T>
using ptr_type = std::shared_ptr<T>;

template <typename T>
using make_ptr_functor_t = make_shared_functor_t<T>;
#endif

using Dense32CountSketch = krowkee::sketch::CountSketch<krowkee::sketch::Dense,
                                                        std::int32_t, ptr_type>;

using MapSparse32CountSketch =
    krowkee::sketch::CountSketch<krowkee::sketch::MapSparse32, std::int32_t,
                                 ptr_type>;

using MapPromotable32CountSketch =
    krowkee::sketch::CountSketch<krowkee::sketch::MapPromotable32, std::int32_t,
                                 ptr_type>;

#if __has_include(<boost/container/flat_map.hpp>)
using FlatMapSparse32CountSketch =
    krowkee::sketch::CountSketch<krowkee::sketch::FlatMapSparse32, std::int32_t,
                                 ptr_type>;

using FlatMapPromotable32CountSketch =
    krowkee::sketch::CountSketch<krowkee::sketch::FlatMapPromotable32,
                                 std::int32_t, ptr_type>;
#endif

using Dense32FWHT = krowkee::sketch::FWHT<std::int32_t, ptr_type>;
