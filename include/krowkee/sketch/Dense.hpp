// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_SKETCH_DENSE_HPP
#define _KROWKEE_SKETCH_DENSE_HPP

#if __has_include(<cereal/types/vector.hpp>)
#include <cereal/types/vector.hpp>
#endif

#include <algorithm>
#include <sstream>
#include <vector>

namespace krowkee {
namespace sketch {

/// good reference for operator declarations:
/// https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading

/**
 * General Dense Sketch
 *
 * Implements the container handling infrastructure of any sketch whose atomic
 * elements include a fixed size set of  of register values and supporting a
 * merge operation consisting of the application of an element-wise operator on
 * pairs of register vectors.
 */
template <typename RegType, typename MergeOp>
class Dense {
 public:
  typedef std::vector<RegType>    col_t;
  typedef Dense<RegType, MergeOp> dense_t;

 protected:
  col_t _registers;

 public:
  template <typename... Args>
  Dense(const std::size_t range_size, const Args &...args) {
    _registers.resize(range_size);
  }

  // copy constructor
  Dense(const dense_t &rhs) : _registers(rhs._registers) {}

  // default constructor
  Dense() {}

  // // move constructor
  // Dense(dense_t &&rhs) : dense_t() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  friend void swap(dense_t &lhs, dense_t &rhs) {
    std::swap(lhs._registers, rhs._registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/types/vector.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_registers);
  }
#endif

  //////////////////////////////////////////////////////////////////////////////
  // Compactify
  //////////////////////////////////////////////////////////////////////////////

  void compactify() {}

  //////////////////////////////////////////////////////////////////////////////
  // Erase
  //////////////////////////////////////////////////////////////////////////////

  inline void erase(const std::uint64_t index) {}

  //////////////////////////////////////////////////////////////////////////////
  // Merge operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Merge other Dense registers into `this`.
   *
   * MergeOp determines how register lists are combined .For linear sketches,
   * merge amounts to the element-wise addition of register arrays.
   *
   * @param rhs the other Dense. Care must be taken to ensure that
   *     one does not merge sketches of different types.
   *
   * @throws std::invalid_argument if the register sizes do not match.
   */
  inline void merge(const dense_t &rhs) {
    if (size() != rhs.size()) {
      std::stringstream ss;
      ss << "error: attempting to merge embedding 1 of dimension " << size()
         << " with embedding 2 of dimension " << rhs.size();
      throw std::invalid_argument(ss.str());
    }
    std::transform(std::begin(_registers), std::end(_registers),
                   std::begin(rhs._registers), std::begin(_registers),
                   MergeOp());
  }

  /**
   * Operator overload for convenience for embeddings without additional
   * consistency checks.
   *
   * @param rhs the other Dense. Care must be taken to ensure that
   *     one does not merge subspace embeddings of different types.
   */
  dense_t &operator+=(const dense_t &rhs) {
    merge(rhs);
    return *this;
  }

  inline friend dense_t operator+(dense_t lhs, const dense_t &rhs) {
    lhs += rhs;
    return lhs;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  constexpr typename col_t::iterator begin() { return std::begin(_registers); }
  constexpr typename col_t::const_iterator begin() const {
    return std::cbegin(_registers);
  }
  constexpr typename col_t::const_iterator cbegin() const {
    return std::cbegin(_registers);
  }
  constexpr typename col_t::iterator end() { return std::end(_registers); }
  constexpr typename col_t::const_iterator end() const {
    return std::cend(_registers);
  }
  constexpr typename col_t::const_iterator cend() {
    return std::cend(_registers);
  }

  constexpr const RegType &operator[](const std::uint64_t index) const {
    return _registers.get(index);
  }

  RegType &operator[](const std::uint64_t index) {
    return _registers.at(index);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  static inline std::string name() { return "Dense"; }

  static inline std::string full_name() { return name(); }

  constexpr bool is_sparse() const { return false; }

  constexpr std::size_t size() const { return _registers.size(); }

  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  constexpr std::size_t get_compaction_threshold() const { return 0; }

  const col_t get_registers() const { return _registers; }

  col_t get_register_vector() const { return _registers; }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////
  constexpr bool same_registers(const dense_t &rhs) const {
    return _registers == rhs._registers;
  }

  // template <typename RT>
  friend constexpr bool operator==(const dense_t &lhs, const dense_t &rhs) {
    return lhs.same_registers(rhs);
  }
  // template <typename RT>
  friend constexpr bool operator!=(const dense_t &lhs, const dense_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Assignment
  //////////////////////////////////////////////////////////////////////////////
  /**
   * copy-and-swap assignment operator
   *
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   */
  dense_t &operator=(dense_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////

  friend std::ostream &operator<<(std::ostream &os, const dense_t &sk) {
    int idx = 0;
    for_each(sk, [&](const auto &p) {
      if (idx != 0) {
        os << " ";
      }
      os << "(" << idx++ << "," << std::int64_t(p) << ")";
    });
    return os;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accumulation
  //////////////////////////////////////////////////////////////////////////////

  template <typename RetType>
  friend RetType accumulate(const dense_t &sk, const RetType init) {
    return std::accumulate(std::cbegin(sk), std::cend(sk), init);
  }

  template <typename Func>
  friend void for_each(const dense_t &sk, const Func &func) {
    std::for_each(std::cbegin(sk._registers), std::cend(sk._registers), func);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif