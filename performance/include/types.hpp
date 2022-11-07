// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <parameters.hpp>

#include <map>
#include <set>
#include <vector>

template <typename KeyType>
void _check_bounds(KeyType idx, std::size_t max) {
  if (idx >= max) {
    throw std::out_of_range("insert index too large!");
  }
  if (idx < 0) {
    throw std::out_of_range("insert index too small!");
  }
}

template <typename ValueType>
struct vector_t {
  using value_t = ValueType;
  vector_t(const parameters_t &params) : _hist(params.domain_size) {}
  vector_t(const parameters_t &params, const value_t &default_value)
      : _hist(params.domain_size, default_value) {}

  static std::string      name() { return "std::vector"; }
  constexpr std::uint64_t size() const { return _hist.size(); }
  template <typename KeyType>
  constexpr void check_bounds(KeyType idx) const {
    _check_bounds(idx, size());
  }

 protected:
  std::vector<value_t> _hist;
};

template <typename KeyType, typename ValueType>
struct map_t {
  using key_t   = KeyType;
  using value_t = ValueType;
  map_t(const parameters_t &params) : _size(params.domain_size), _hist() {}

  static std::string      name() { return "std::map"; }
  constexpr std::uint64_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::map<key_t, value_t> _hist;
  std::size_t              _size;
};

template <typename KeyType>
struct set_t {
  using key_t = KeyType;
  set_t(const parameters_t &params) : _size(params.domain_size), _hist() {}

  static std::string      name() { return "std::set"; }
  constexpr std::uint64_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::set<key_t> _hist;
  std::size_t     _size;
};
