// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_SKETCH_SPARSE_HPP
#define _KROWKEE_SKETCH_SPARSE_HPP

#include <krowkee/container/compacting_map.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

namespace krowkee {
namespace sketch {

/**
 * General Sparse Sketch
 *
 * Implements the container handling infrastructure of any sketch whose atomic
 * elements can be stored as sorted sparse index-value pairs into a notional
 * register list of fixed size and supporting a merge operation consisting of
 * the union of their index-value pair lists  with the application of an
 * element-wise operator on pairs with matching indices.
 */
template <typename RegType, typename MergeOp,
          template <typename, typename> class MapType, typename KeyType>
class Sparse {
 public:
  typedef krowkee::container::compacting_map<KeyType, RegType, MapType> col_t;
  typedef typename col_t::vec_iter_t                 vec_iter_t;
  typedef typename col_t::vec_citer_t                vec_citer_t;
  typedef typename col_t::pair_t                     pair_t;
  typedef typename col_t::map_t                      map_t;
  typedef Sparse<RegType, MergeOp, MapType, KeyType> sparse_t;

 protected:
  col_t       _registers;
  std::size_t _range_size;

 public:
  template <typename... Args>
  Sparse(const std::size_t range_size, const std::size_t compaction_threshold,
         const Args &...args)
      : _range_size(range_size), _registers(compaction_threshold) {}

  /**
   * Copy constructor.
   */
  Sparse(const sparse_t &rhs)
      : _range_size(rhs._range_size), _registers(rhs._registers) {}

  // default constructor
  Sparse() {}

  // // move constructor
  // Sparse(sparse_t &&rhs) : sparse_t() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * Swap boilerplate.
   *
   * For some reason, calling std::swap on the registers here causes a segfault
   * in Distributed::data_t::swap, but not Sketch::swap. Peculiar.
   */
  friend void swap(sparse_t &lhs, sparse_t &rhs) {
    std::swap(lhs._range_size, rhs._range_size);
    swap(lhs._registers, rhs._registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

  template <class Archive>
  void serialize(Archive &archive) {
    archive(_range_size, _registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  inline bool is_compact() const { return _registers.is_compact(); }

  void compactify() { _registers.compactify(); }

  //////////////////////////////////////////////////////////////////////////////
  // Erase
  //////////////////////////////////////////////////////////////////////////////

  inline void erase(const std::uint64_t index) { _registers.erase(index); }

  //////////////////////////////////////////////////////////////////////////////
  // Merge operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Merge other Sparse registers into `this`.
   *
   * MergeOp determines how register lists are combined .For linear
   * sketches, merge amounts to the element-wise addition of register
   * arrays.
   *
   * @param rhs the other Sparse. Care must be taken to ensure that
   *     one does not merge sketches of different types.
   *
   * @throw std::logic_error if invoked on uncompacted sketches.
   */
  inline void merge(const sparse_t &rhs) {
    _registers.merge(rhs._registers, MergeOp());
  }

  /**
   * Operator overload for convenience for embeddings without additional
   * consistency checks.
   *
   * @param rhs the other Sparse. Care must be taken to ensure that
   *     one does not merge subspace embeddings of different types.
   */
  sparse_t &operator+=(const sparse_t &rhs) {
    merge(rhs);
    return *this;
  }

  inline friend sparse_t operator+(sparse_t lhs, const sparse_t &rhs) {
    lhs += rhs;
    return lhs;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  constexpr auto begin() { return std::begin(_registers); }
  constexpr auto begin() const { return std::cbegin(_registers); }
  constexpr auto cbegin() const { return std::cbegin(_registers); }
  constexpr auto end() { return std::end(_registers); }
  constexpr auto end() const { return std::cend(_registers); }
  constexpr auto cend() { return std::cend(_registers); }

  constexpr const RegType &operator[](const std::uint64_t index) const {
    return _registers.get(index);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accessors
  //////////////////////////////////////////////////////////////////////////////

  RegType &operator[](const std::uint64_t index) { return _registers[index]; }

  RegType &at(const std::uint64_t index) { return _registers.at(index); }

  RegType &at(const std::uint64_t index, const RegType def) {
    return _registers.at(index, def);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  static inline std::string name() { return "Sparse"; }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << krowkee::hash::type_name<map_t>();
    return ss.str();
  }

  constexpr bool is_sparse() const { return true; }

  constexpr std::size_t size() const { return _registers.size(); }

  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  constexpr std::size_t range_size() const { return _range_size; }

  constexpr std::size_t compaction_threshold() const {
    return _registers.compaction_threshold();
  }

  std::vector<RegType> register_vector() const {
    std::vector<RegType> ret(range_size());
    std::for_each(std::cbegin(_registers), std::cend(_registers),
                  [&ret](const std::pair<KeyType, RegType> &elem) {
                    ret[elem.first] = elem.second;
                  });
    return ret;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////
  constexpr bool same_parameters(const sparse_t &rhs) const {
    return _range_size == rhs._range_size;
  }

  constexpr bool same_registers(const sparse_t &rhs) const {
    return _registers == rhs._registers;
  }

  // template <typename RT>
  friend constexpr bool operator==(const sparse_t &lhs, const sparse_t &rhs) {
    return lhs.same_parameters(rhs) && lhs.same_registers(rhs);
  }
  // template <typename RT>
  friend constexpr bool operator!=(const sparse_t &lhs, const sparse_t &rhs) {
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
  sparse_t &operator=(sparse_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }
  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Read (compacted) state out to ostream.
   *
   * @throw std::logic_error if invoked on uncompacted sketch.
   */
  friend std::ostream &operator<<(std::ostream &os, const sparse_t &sk) {
    if (sk.is_compact() == false) {
      throw std::logic_error("Bad attempt to write uncompacted map!");
    }
    // os << sk._registers.print_state();
    bool first(true);
    for_each(sk, [&](const auto &p) {
      if (first == false) {
        os << " ";
      } else {
        first == false;
      }
      os << "(" << std::int64_t(p.first) << "," << int(p.second) << ") ";
    });
    return os;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accumulation
  //////////////////////////////////////////////////////////////////////////////

  template <typename RetType>
  friend RetType accumulate(const sparse_t &sk, const RetType init) {
    return std::accumulate(std::cbegin(sk._registers), std::cend(sk._registers),
                           init, [](const RetType val, const pair_t &itr) {
                             return RetType(val + itr.second);
                           });
  }

  template <typename Func>
  friend void for_each(const sparse_t &sk, const Func &func) {
    std::for_each(std::cbegin(sk._registers), std::cend(sk._registers), func);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif