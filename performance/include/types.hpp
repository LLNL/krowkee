// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <map>
#include <set>
#include <sstream>
#include <vector>

template <typename KeyType>
void _check_bounds(KeyType idx, std::size_t max) {
  if (idx >= max) {
    std::stringstream ss;
    ss << "insert index " << idx << " is larger than the maximum (" << max
       << ")!";
    throw std::out_of_range(ss.str());
  }
  if (idx < 0) {
    std::stringstream ss;
    ss << "insert index " << idx << " is < 0!";
    throw std::out_of_range(ss.str());
  }
}

template <typename ValueType>
struct Vector {
  using value_t = ValueType;
  Vector(const std::size_t size) : _vec(size) {}
  Vector(const std::size_t size, const value_t &default_value)
      : _vec(size, default_value) {}

  static std::string    name() { return "std::vector"; }
  constexpr std::size_t size() const { return _vec.size(); }
  template <typename KeyType>
  constexpr void check_bounds(KeyType idx) const {
    _check_bounds(idx, size());
  }

 protected:
  std::vector<value_t> _vec;
};

template <typename KeyType, typename ValueType>
struct Map {
  using key_t   = KeyType;
  using value_t = ValueType;
  Map(const std::size_t size) : _size(size), _map() {}

  static std::string    name() { return "std::map"; }
  constexpr std::size_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::map<key_t, value_t> _map;
  std::size_t              _size;
};

template <typename KeyType>
struct Set {
  using key_t = KeyType;
  Set(const std::size_t size) : _size(size), _set() {}

  static std::string    name() { return "std::set"; }
  constexpr std::size_t size() const { return _size; }
  constexpr void check_bounds(key_t idx) const { _check_bounds(idx, size()); }

 protected:
  std::set<key_t> _set;
  std::size_t     _size;
};
