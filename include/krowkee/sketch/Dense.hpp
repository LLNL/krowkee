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
 * @brief General Dense Sketch Container
 *
 * Implements the container handling infrastructure of any sketch whose atomic
 * elements include a fixed size set of  of register values and supporting a
 * merge operation consisting of the application of an element-wise operator on
 * pairs of register vectors.
 *
 * @tparam RegType The type held by each register.
 * @tparam MergeOp An element-wise merge operator to combine two sketches,
 * templated on RegType.
 */
template <typename RegType, typename MergeOp>
class Dense {
 public:
  /** Alias for the register collection type */
  typedef std::vector<RegType> col_t;
  /** Alias for the fully-tempalted Dense type*/
  typedef Dense<RegType, MergeOp> dense_t;

 protected:
  col_t _registers;

 public:
  /**
   * @brief Construct a new Dense container object
   *
   * @tparam Args Other args (ignored)
   * @param range_size The number of registers, equal to the range size of the
   * sketch functor.
   * @param args Ignored by Dense.
   */
  template <typename... Args>
  Dense(const std::size_t range_size, const Args &...args)
      : _registers(range_size) {}

  /**
   * @brief Copy constructor.
   *
   * @param rhs The base Dense container to copy.
   */
  Dense(const dense_t &rhs) : _registers(rhs._registers) {}

  /**
   * @brief Construct a new Dense object
   *
   * @note Only used for move constructor.
   */
  Dense() {}

  // // move constructor
  // Dense(dense_t &&rhs) : dense_t() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Swap two Dense container
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   */
  friend void swap(dense_t &lhs, dense_t &rhs) {
    std::swap(lhs._registers, rhs._registers);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/types/vector.hpp>)
  /**
   * @brief Serialize Dense object to/from `cereal` archive.
   *
   * @tparam Archive `cereal` archive type.
   * @param archive The `cereal` archive to which to serialize the sketch.
   */
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
   * @brief Merge other Dense registers into `this`.
   *
   * @param rhs the other Dense. Care must be taken to ensure that one does not
   * merge sketches of different types.
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
   * @brief Operator overload for convenience for embeddings without additional
   * consistency checks.
   *
   * @param rhs the other Dense. Care must be taken to ensure that
   *     one does not merge subspace embeddings of different types.
   * @return dense_t& `this` Dense, having been merged with `rhs`.
   */
  dense_t &operator+=(const dense_t &rhs) {
    merge(rhs);
    return *this;
  }

  /**
   * @brief Merge two Dense containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return dense_t The merge of the two container objects.
   */
  inline friend dense_t operator+(dense_t lhs, const dense_t &rhs) {
    lhs += rhs;
    return lhs;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  /** Mutable begin iterator. */
  constexpr typename col_t::iterator begin() { return std::begin(_registers); }
  /** Const begin iterator. */
  constexpr typename col_t::const_iterator begin() const {
    return std::cbegin(_registers);
  }
  /** Const begin iterator. */
  constexpr typename col_t::const_iterator cbegin() const {
    return std::cbegin(_registers);
  }
  /** Mutable end iterator. */
  constexpr typename col_t::iterator end() { return std::end(_registers); }
  /** Const end iterator. */
  constexpr typename col_t::const_iterator end() const {
    return std::cend(_registers);
  }
  /** Const end iterator. */
  constexpr typename col_t::const_iterator cend() {
    return std::cend(_registers);
  }

  /**
   * @brief Const access Dense at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @return constexpr const RegType& A const reference to `_registers[index]`.
   */
  constexpr const RegType &operator[](const std::uint64_t index) const {
    return _registers.get(index);
  }

  /**
   * @brief Access Dense at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `range_size`.
   * @return RegType& A reference to `_registers[index]`.
   */
  RegType &operator[](const std::uint64_t index) {
    return _registers.at(index);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Returns a description of the type of container.
   *
   * @return std::string "Dense"
   */
  static inline std::string name() { return "Dense"; }

  /**
   * @brief Returns a description of the fully-qualified type of container.
   *
   * @return std::string "Dense"
   */
  static inline std::string full_name() { return name(); }

  /** Dense is also not Sparse. */
  constexpr bool is_sparse() const { return false; }

  /** The size of the registers vector. */
  constexpr std::size_t size() const { return _registers.size(); }

  /** The number of bytes used by each register. */
  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  constexpr std::size_t compaction_threshold() const { return 0; }

  /**
   * @brief Get a copy of the raw registers vector.
   *
   * @return const col_t The register vector.
   */
  const col_t get_registers() const { return _registers; }

  /**
   * @brief Get a copy of the raw registers vector.
   *
   * @return const col_t The register vector.
   */
  col_t register_vector() const { return _registers; }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Checks whether another Dense container has the same register state.
   *
   * @param rhs The other container.
   * @return true The registers agree.
   * @return false At least one register disagrees.
   */
  constexpr bool same_registers(const dense_t &rhs) const {
    return _registers == rhs._registers;
  }

  /**
   * @brief Checks equality between two Dense containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return true The registers agree.
   * @return false At least one register disagrees.
   */
  friend constexpr bool operator==(const dense_t &lhs, const dense_t &rhs) {
    return lhs.same_registers(rhs);
  }

  /**
   * @brief CHecks inequality between two Dense containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return true At least one register disagrees.
   * @return false The registers agree.
   */
  friend constexpr bool operator!=(const dense_t &lhs, const dense_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Assignment
  //////////////////////////////////////////////////////////////////////////////
  /**
   * copy-and-swap assignment operator
   *
   *
   */

  /**
   * @brief Copy-and-swap assignment operator
   *
   * @note
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   *
   * @param rhs The other container.
   * @return dense_t& `this`, having been swapped with `rhs`.
   */
  dense_t &operator=(dense_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Serialize a Dense container to human-readable output stream.
   *
   * @note Intended for debugging only.
   *
   * @param os The output stream.
   * @param sk The Dense object.
   * @return std::ostream& The new stream state.
   */
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

  /**
   * @brief Accumulate sum of register values.
   *
   * @tparam RetType The value type to return.
   * @param sk The sketch to accumulate.
   * @param init Initial value of return.
   * @return RetType The sum over all register values + `init`.
   */
  template <typename RetType>
  friend RetType accumulate(const dense_t &sk, const RetType init) {
    return std::accumulate(std::cbegin(sk), std::cend(sk), init);
  }

  /**
   * @brief Apply a given function to all register values.
   *
   * @tparam Func The (probably lambda) function type.
   * @param sk The Dense object.
   * @param func The particular function to apply.
   */
  template <typename Func>
  friend void for_each(const dense_t &sk, const Func &func) {
    std::for_each(std::cbegin(sk._registers), std::cend(sk._registers), func);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif