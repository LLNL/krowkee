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
 * @brief General Sparse Sketch container.
 *
 * Implements the container handling infrastructure of any sketch whose atomic
 * elements can be stored as sorted sparse index-value pairs into a notional
 * register list of fixed size and supporting a merge operation consisting of
 * the union of their index-value pair lists  with the application of an
 * element-wise operator on pairs with matching indices.
 *
 * @tparam RegType The type held by each register.
 * @tparam MergeOp An element-wise merge operator to combine two sketches,
 * templated on RegType.
 * @tparam MapType The underlying map class to use for buffered updates to the
 * underlying compacting_map.
 * @tparam KeyType The key type to use for this underlying compacting map.
 */
template <typename RegType, typename MergeOp,
          template <typename, typename> class MapType, typename KeyType>
class Sparse {
 public:
  /** Alias for the fully-templated register compacting map type. */
  typedef krowkee::container::compacting_map<KeyType, RegType, MapType> col_t;
  typedef typename col_t::vec_iter_t  vec_iter_t;
  typedef typename col_t::vec_citer_t vec_citer_t;
  typedef typename col_t::pair_t      pair_t;
  typedef typename col_t::map_t       map_t;
  /** Alias for the fully-templated Sparse type. */
  typedef Sparse<RegType, MergeOp, MapType, KeyType> sparse_t;

 protected:
  col_t       _registers;
  std::size_t _range_size;

 public:
  /**
   * @brief Construct a new Sparse container object
   *
   * @tparam Args Other args (ignored)
   * @param range_size The number of registers, equal to the range size of the
   * sketch functor.
   * @param compaction_threshold The size at which the compacting_map buffer
   * triggers compaction.
   * @param args Ignored by Dense.
   */
  template <typename... Args>
  Sparse(const std::size_t range_size, const std::size_t compaction_threshold,
         const Args &...args)
      : _range_size(range_size), _registers(compaction_threshold) {}

  /**
   * @brief Copy constructor.
   *
   * @param rhs The base Sparse container to copy.
   */
  Sparse(const sparse_t &rhs)
      : _range_size(rhs._range_size), _registers(rhs._registers) {}

  /**
   * @brief Default constructor for Dense
   *
   * @note Only used for move constructor.
   */
  Sparse() {}

  // // move constructor
  // Sparse(sparse_t &&rhs) : sparse_t() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * Swap boilerplate.
   *
   */
  /**
   * @brief Swap two Sparse containers.
   *
   * @note For some reason, calling std::swap on the registers here causes a
   * segfault in Distributed::data_t::swap, but not Sketch::swap. Peculiar.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   */
  friend void swap(sparse_t &lhs, sparse_t &rhs) {
    std::swap(lhs._range_size, rhs._range_size);
    swap(lhs._registers, rhs._registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Serialize Sparse object to/from `cereal` archive.
   *
   * @tparam Archive `cereal` archive type.
   * @param archive The `cereal` archive to which to serialize the sketch.
   */
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_range_size, _registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Checks whether the compacting sketch is compact.
   *
   * @return true The update buffer is empty.
   * @return false The update buffer is not empty.
   */
  inline bool is_compact() const { return _registers.is_compact(); }

  /** Flush the update buffer. */
  void compactify() { _registers.compactify(); }

  //////////////////////////////////////////////////////////////////////////////
  // Clear & Empty
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Remove all information from sketch.
   */
  void clear() { _registers.clear(); }

  /**
   * @brief Check if registers contain any information.
   */
  bool empty() const { return _registers.empty(); }

  //////////////////////////////////////////////////////////////////////////////
  // Erase
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Erase a map index.
   *
   * @param index The map index to erase.
   */
  inline void erase(const std::uint64_t index) { _registers.erase(index); }

  //////////////////////////////////////////////////////////////////////////////
  // Merge operators
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Merge other Sparse registers into `this`.
   *
   * MergeOp determines how register lists are combined .For linear
   * sketches, merge amounts to the element-wise addition of register
   * arrays.
   *
   *
   * @param rhs the other Sparse. Care must be taken to ensure that one does not
   * merge sketches of different types.
   * @throw std::logic_error if invoked on uncompacted sketches.
   */
  inline void merge(const sparse_t &rhs) {
    _registers.merge(rhs._registers, MergeOp());
  }

  /**
   *
   *
   */

  /**
   * @brief Operator overload for convenience for embeddings without additional
   * consistency checks.
   *
   * @param rhs the other Sparse. Care must be taken to ensure that one does not
   * merge subspace embeddings of different types.
   * @return sparse_t& `this` Sparse, having been merged with `rhs`.
   */
  sparse_t &operator+=(const sparse_t &rhs) {
    merge(rhs);
    return *this;
  }

  /**
   * @brief Merge two Sparse containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return sparse_t The merge of the two containers.
   */
  inline friend sparse_t operator+(sparse_t lhs, const sparse_t &rhs) {
    lhs += rhs;
    return lhs;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  /** Mutable begin iterator. */
  constexpr auto begin() { return std::begin(_registers); }
  /** Const begin iterator. */
  constexpr auto begin() const { return std::cbegin(_registers); }
  /** Const begin iterator. */
  constexpr auto cbegin() const { return std::cbegin(_registers); }
  /** Mutable end iterator. */
  constexpr auto end() { return std::end(_registers); }
  /** Const end iterator. */
  constexpr auto end() const { return std::cend(_registers); }
  /** Const end iterator. */
  constexpr auto cend() { return std::cend(_registers); }

  //////////////////////////////////////////////////////////////////////////////
  // Accessors
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Const access Sparse at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @return constexpr const RegType& A const refernce to `_registers[index]`.
   */
  constexpr const RegType &operator[](const std::uint64_t index) const {
    return _registers.get(index);
  }

  /**
   * @brief Access Sparse at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @return RegType& A refernce to `_registers[index]`.
   */
  RegType &operator[](const std::uint64_t index) { return _registers[index]; }

  /**
   * @brief Access Sparse at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @return RegType& A refernce to `_registers.at(index)`.
   */
  RegType &at(const std::uint64_t index) { return _registers.at(index); }

  /**
   * @brief Access Sparse at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @param def The default value to return if the index does not exist.
   * @return RegType& A refernce to `_registers.at(index)`.
   */
  RegType &at(const std::uint64_t index, const RegType def) {
    return _registers.at(index, def);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Returns a description of the type of container.
   *
   * @return std::string "Sparse"
   */
  static inline std::string name() { return "Sparse"; }

  /**
   * @brief Returns a description of the fully-qualified type of container
   *
   * @return std::string "Sparse" plus the full name of the fully-templated
   * compacting_map type.
   */
  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << krowkee::hash::type_name<map_t>();
    return ss.str();
  }

  /** Sparse is always Sparse. */
  constexpr bool is_sparse() const { return true; }

  /** The current size of the compacting_map. */
  constexpr std::size_t size() const { return _registers.size(); }

  /** The number of bytes used by each register. */
  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  /** The maximum possible size of the compacting_map. */
  constexpr std::size_t range_size() const { return _range_size; }

  /** The size at which the compaction buffer will flush. */
  constexpr std::size_t compaction_threshold() const {
    return _registers.compaction_threshold();
  }

  /**
   * @brief Get a copy of the raw (sparse) register vector
   *
   * @return std::vector<RegType> The register vector, with zeros for unset
   * indices.
   * @throws std::logic_error if the object is not compact.
   */
  std::vector<RegType> register_vector() const {
    if (is_compact() == false) {
      throw std::logic_error("Bad attempt to export uncompacted map!");
    }
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
  /**
   * @brief Checks whether another Sparse container has the same range_size.
   *
   * @param rhs The other container.
   * @return true The sizes agree.
   * @return false The sizes disagree.
   */
  constexpr bool same_parameters(const sparse_t &rhs) const {
    return _range_size == rhs._range_size;
  }

  /**
   * @brief Checks whether another Sparse container has the same register state.
   *
   * @param rhs The other container.
   * @return true The registers agree.
   * @return false At least one register disagrees.
   */
  constexpr bool same_registers(const sparse_t &rhs) const {
    return _registers == rhs._registers;
  }

  /**
   * @brief Checks equality between two Sparse containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return true The registers agree.
   * @return false At least one register disagrees.
   */
  friend constexpr bool operator==(const sparse_t &lhs, const sparse_t &rhs) {
    return lhs.same_parameters(rhs) && lhs.same_registers(rhs);
  }

  /**
   * @brief CHecks inequality between two Sparse containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return true At least one register disagrees.
   * @return false The registers agree.
   */
  friend constexpr bool operator!=(const sparse_t &lhs, const sparse_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Assignment
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Copy-and-swap assignment operator
   *
   * @note
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   *
   * @param rhs The other container.
   * @return sparse_t& `this`, having been swapped with `rhs`.
   */
  sparse_t &operator=(sparse_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }
  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Serialize a Sparse container to human-readable output stream.
   *
   * Output format is a list of (key, value) pairs, skipping empty registers.
   *
   * @note Intended for debugging only.
   *
   * @param os The output stream.
   * @param sk The Sparse object.
   * @return std::ostream& The new stream state.
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

  /**
   * @brief Accumulate sum of register values.
   *
   * @tparam RetType The value type to return.
   * @param sk The Sparse to accumulate.
   * @param init Initial value of return.
   * @return RetType The sum over all register values + `init`.
   */
  template <typename RetType>
  friend RetType accumulate(const sparse_t &sk, const RetType init) {
    return std::accumulate(std::cbegin(sk._registers), std::cend(sk._registers),
                           init, [](const RetType val, const pair_t &itr) {
                             return RetType(val + itr.second);
                           });
  }

  /**
   * @brief Apply a given function to all register values.
   *
   * @tparam Func The (probably lambda) function type.
   * @param sk The Sparse object.
   * @param func The particular function to apply.
   */
  template <typename Func>
  friend void for_each(const sparse_t &sk, const Func &func) {
    std::for_each(std::cbegin(sk._registers), std::cend(sk._registers), func);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif