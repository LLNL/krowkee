// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/sketch.hpp>
#include <krowkee/util/runtime.hpp>

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
struct make_ygm_ptr_functor {
  using ptr_type = ygm::ygm_ptr<T>;

  make_ygm_ptr_functor() {}

  template <typename... Args>
  ptr_type operator()(Args... args) {
    sptrs.push_back(std::make_unique<T>(args...));
    return _make_ygm_ptr<T>(*(sptrs.back()));
  }

  static constexpr std::string name() { return "ygm::ygm_ptr"; }

 private:
  std::vector<std::unique_ptr<T>> sptrs;
};

template <typename T>
using make_ptr_functor = make_ygm_ptr_functor<T>;
#else

template <typename T>
using ptr_type = std::shared_ptr<T>;

template <typename T>
using make_ptr_functor = krowkee::make_shared_functor<T>;
#endif

using register_type = float;

template <std::size_t RangeSize, std::size_t ReplicationCount>
using Dense32SparseJLT =
    krowkee::sketch::SparseJLT<krowkee::sketch::Dense, register_type, RangeSize,
                               ReplicationCount, ptr_type>;

template <std::size_t RangeSize, std::size_t ReplicationCount>
using MapSparse32SparseJLT =
    krowkee::sketch::SparseJLT<krowkee::sketch::MapSparse32, register_type,
                               RangeSize, ReplicationCount, ptr_type>;

template <std::size_t RangeSize, std::size_t ReplicationCount>
using MapPromotable32SparseJLT =
    krowkee::sketch::SparseJLT<krowkee::sketch::MapPromotable32, register_type,
                               RangeSize, ReplicationCount, ptr_type>;

#if __has_include(<boost/container/flat_map.hpp>)
template <std::size_t RangeSize, std::size_t ReplicationCount>
using FlatMapSparse32SparseJLT =
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapSparse32, register_type,
                               RangeSize, ReplicationCount, ptr_type>;

template <std::size_t RangeSize, std::size_t ReplicationCount>
using FlatMapPromotable32SparseJLT =
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapPromotable32,
                               register_type, RangeSize, ReplicationCount,
                               ptr_type>;
#endif

template <std::size_t RangeSize, std::size_t ReplicationCount>
using Dense32FWHT =
    krowkee::sketch::FWHT<register_type, RangeSize, ReplicationCount, ptr_type>;
