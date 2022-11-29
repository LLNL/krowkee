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
  vector_t(const parameters_t &params) : _vec(params.range_size) {}
  vector_t(const parameters_t &params, const value_t &default_value)
      : _vec(params.range_size, default_value) {}

  static std::string      name() { return "std::vector"; }
  constexpr std::uint64_t size() const { return _vec.size(); }
  template <typename KeyType>
  constexpr void check_bounds(KeyType idx) const {
    _check_bounds(idx, size());
  }

 protected:
  std::vector<value_t> _vec;
};

template <typename KeyType, typename ValueType>
struct map_t {
  using key_t   = KeyType;
  using value_t = ValueType;
  map_t(const parameters_t &params) : _size(params.range_size), _map() {}

  static std::string      name() { return "std::map"; }
  constexpr std::uint64_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::map<key_t, value_t> _map;
  std::size_t              _size;
};

template <typename KeyType>
struct set_t {
  using key_t = KeyType;
  set_t(const parameters_t &params) : _size(params.range_size), _set() {}

  static std::string      name() { return "std::set"; }
  constexpr std::uint64_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::set<key_t> _set;
  std::size_t     _size;
};
