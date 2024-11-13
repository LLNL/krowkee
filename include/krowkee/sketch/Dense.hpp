// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

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
 * @tparam MergeOp An template merge operator to combine two sketches.
 */
template <typename RegType, template <typename> class MergeOp>
class Dense {
 public:
  using register_type  = RegType;
  using registers_type = std::vector<register_type>;
  using merge_type     = MergeOp<register_type>;
  using self_type      = Dense<register_type, MergeOp>;

 protected:
  registers_type _registers;

 public:
  /**
   * @brief Construct a new Dense container object
   *
   * @tparam Args Other args (ignored)
   * @param size The number of registers, equal to the range size of the sketch
   * functor times its replication count.
   * @param args Ignored by Dense.
   */
  template <typename... Args>
  Dense(const std::size_t size, const Args &...args) : _registers(size) {}

  /**
   * @brief Copy constructor.
   *
   * @param rhs The base Dense container to copy.
   */
  Dense(const self_type &rhs) : _registers(rhs._registers) {}

  /**
   * @brief Default constructor for Dense
   *
   * @note Only used for move constructor.
   */
  Dense() {}

  // // move constructor
  // Dense(self_type &&rhs) : self_type() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Swap two Dense containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   */
  friend void swap(self_type &lhs, self_type &rhs) {
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

  /**
   * @brief A no-op for dense containers.
   */
  void compactify() {}

  //////////////////////////////////////////////////////////////////////////////
  // Clear & Empty
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Set all registers to 0.
   */
  void clear() { std::fill(std::begin(_registers), std::end(_registers), 0); }

  /**
   * @brief Check if all registers are 0.
   */
  bool empty() const {
    return std::all_of(std::begin(_registers), std::end(_registers),
                       [](const auto i) { return i == 0; });
  }

  //////////////////////////////////////////////////////////////////////////////
  // Erase
  //////////////////////////////////////////////////////////////////////////////

  constexpr void erase(const std::uint64_t index) {}

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
  constexpr void merge(const self_type &rhs) {
    if (size() != rhs.size()) {
      std::stringstream ss;
      ss << "error: attempting to merge embedding 1 of dimension " << size()
         << " with embedding 2 of dimension " << rhs.size();
      throw std::invalid_argument(ss.str());
    }
    std::transform(std::begin(_registers), std::end(_registers),
                   std::begin(rhs._registers), std::begin(_registers),
                   merge_type());
  }

  /**
   * @brief Operator overload for convenience for embeddings without additional
   * consistency checks.
   *
   * @param rhs the other Dense. Care must be taken to ensure that
   *     one does not merge subspace embeddings of different types.
   * @return self_type& `this` Dense, having been merged with `rhs`.
   */
  self_type &operator+=(const self_type &rhs) {
    merge(rhs);
    return *this;
  }

  /**
   * @brief Merge two Dense containers.
   *
   * @param lhs The left-hand container.
   * @param rhs The right-hand container.
   * @return self_type The merge of the two container objects.
   */
  constexpr friend self_type operator+(self_type lhs, const self_type &rhs) {
    lhs += rhs;
    return lhs;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  /** Mutable begin iterator. */
  constexpr typename registers_type::iterator begin() {
    return std::begin(_registers);
  }
  /** Const begin iterator. */
  constexpr typename registers_type::const_iterator begin() const {
    return std::cbegin(_registers);
  }
  /** Const begin iterator. */
  constexpr typename registers_type::const_iterator cbegin() const {
    return std::cbegin(_registers);
  }
  /** Mutable end iterator. */
  constexpr typename registers_type::iterator end() {
    return std::end(_registers);
  }
  /** Const end iterator. */
  constexpr typename registers_type::const_iterator end() const {
    return std::cend(_registers);
  }
  /** Const end iterator. */
  constexpr typename registers_type::const_iterator cend() {
    return std::cend(_registers);
  }

  /**
   * @brief Const access Dense at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `size`.
   * @return constexpr const register_type& A const reference to
   * `_registers[index]`.
   */
  constexpr const register_type &operator[](const std::uint64_t index) const {
    return _registers.get(index);
  }

  /**
   * @brief Access Dense at `index`.
   *
   * @param index The index of the underlying vector to index. Must be less than
   * `size`.
   * @return register_type& A reference to `_registers[index]`.
   */
  register_type &operator[](const std::uint64_t index) {
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
  static constexpr std::string name() { return "Dense"; }

  /**
   * @brief Returns a description of the fully-qualified type of container.
   *
   * @return std::string "Dense"
   */
  static constexpr std::string full_name() { return name(); }

  /** Dense is also not Sparse. */
  constexpr bool is_sparse() const { return false; }

  /** The size of the registers vector. */
  constexpr std::size_t size() const { return _registers.size(); }

  /** The size of the registers vector. */
  constexpr std::size_t max_size() const { return _registers.size(); }

  /** The number of bytes used by each register. */
  constexpr std::size_t reg_size() const { return sizeof(register_type); }

  constexpr std::size_t compaction_threshold() const { return 0; }

  /**
   * @brief Get a copy of the raw registers vector.
   *
   * @return const registers_type The register vector.
   */
  const registers_type get_registers() const { return _registers; }

  /**
   * @brief Get a copy of the raw registers vector.
   *
   * @return const registers_type The register vector.
   */
  registers_type register_vector() const { return _registers; }

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
  constexpr bool same_registers(const self_type &rhs) const {
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
  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
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
  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
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
   * @return self_type& `this`, having been swapped with `rhs`.
   */
  self_type &operator=(self_type rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Serialize a Dense container to human-readable output stream.
   *
   * Output format is a space-delmined list of (key, value) pairs.
   *
   * @note Intended for debugging only.
   *
   * @param os The output stream.
   * @param sk The Dense object.
   * @return std::ostream& The new stream state.
   */
  friend std::ostream &operator<<(std::ostream &os, const self_type &sk) {
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
   * @param sk The Dense to accumulate.
   * @param init Initial value of return.
   * @return RetType The sum over all register values + `init`.
   */
  template <typename RetType>
  friend RetType accumulate(const self_type &sk, const RetType init) {
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
  friend void for_each(const self_type &sk, const Func &func) {
    std::for_each(std::cbegin(sk._registers), std::cend(sk._registers), func);
  }
};

}  // namespace sketch
}  // namespace krowkee
