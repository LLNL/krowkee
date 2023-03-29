// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_SKETCH_SKETCH_HPP
#define _KROWKEE_SKETCH_SKETCH_HPP

#if __has_include(<cereal/types/memory.hpp>)
#include <cereal/types/memory.hpp>
#endif

namespace krowkee {
namespace sketch {

/**
 * @brief General Sketch Chassis.
 *
 * Implements the container handling infrastructure of a linear sketch
 * defined by a given sketch functor projecting into a fixed-size set of
 * registers supporting an (element-wise) merge operator. The registers might be
 * Dense, Sparse, or variable, depending on the templated ContainerType.
 *
 * @tparam SketchFunc A sketch functor such as
 * `krowkee::transform::CountSketchFunctor` whose first template parameter is
 * set by `RegType`.
 * @tparam ContainerType A container class such as `krowkee::sketch::Dense`
 * whose first two template parameters are set by `RegType` and
 * `MergeOp<RegType>`, respectively.
 * @tparam MergeOp A bivariate merge operator on arrays of `RegType`s.
 * @tparam RegType The type of to hold in individual registers.
 * @tparam PtrType The type of shared pointer used to wrap the sketch functor.
 * Should be `std::shared_ptr`in shared memory and `ygm::ygm_ptr` in distributed
 * memory.
 * @tparam Args Additional template parameters for `SketchFunc`, such as a hash
 * functor.
 */
template <template <typename, typename...> class SketchFunc,
          template <typename, typename> class ContainerType,
          template <typename> class MergeOp, typename RegType,
          template <typename> class PtrType, typename... Args>
class Sketch {
 public:
  /** Alias for Register type */
  typedef RegType reg_t;
  /** Alias for fully-templated container type */
  typedef ContainerType<RegType, MergeOp<RegType>> container_t;
  /** Alias for fully-templated sketch functor type*/
  typedef SketchFunc<RegType, Args...> sf_t;
  /** Alias for fully-templated sketch functor pointer type*/
  typedef PtrType<sf_t> sf_ptr_t;
  /** Alias for fully-templated self type*/
  typedef Sketch<SketchFunc, ContainerType, MergeOp, RegType, PtrType, Args...>
      sk_t;

 private:
  sf_ptr_t    _sf_ptr;
  container_t _con;

 public:
  /**
   * @brief Construct a new Sketch object
   *
   * @param sf_ptr pointer desired linear sketch functor.
   * @param compaction_threshold threshold at which sparse sketches should
   * `compactify` themselves.
   * @param promotion_threshold threshold at which promotable sketchs should
   * promote themselves.
   */
  Sketch(const sf_ptr_t &sf_ptr, const std::size_t compaction_threshold = 100,
         const std::size_t promotion_threshold = 4096)
      : _con(sf_ptr->range_size(), compaction_threshold, promotion_threshold),
        _sf_ptr(sf_ptr) {}

  /**
   * @brief Copy constructor
   *
   * @param rhs `krowkee::sketch::Sketch` to be copied
   */
  Sketch(const sk_t &rhs) : _con(rhs._con), _sf_ptr(rhs._sf_ptr) {}

  /**
   * @brief default constructor (only use for move constructor!)
   *
   */
  Sketch() {}
  // Sketch() : _con(), _sf_ptr(std::make_shared<sf_t>(1)) {}
  // Sketch() : _con(), _make_def_ptr(), _sf_ptr(_make_def_ptr(1)) {}
  // Sketch() : _con(), _sf_ptr(_make_default_ptr()) {
  //   std::cout << "GETS HERE!!!!!" << std::endl;
  // }
  // Sketch() { std::cout << "GETS HERE!!!!!" << std::endl; }

  // /**
  //  * move constructor
  //  *
  //  * @param rhs krowkee::sketch::Sketch to be destructively copied.
  //  */
  // Sketch(sk_t &&rhs) noexcept : Sketch() { std::swap(*this, rhs); }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/types/memory.hpp>)
  /**
   * @brief Serialize sketch object to/from `cereal` archive.
   *
   * @tparam Archive `cereal` archive type
   * @param archive The `cereal` archive to which to serialize the sketch.
   */
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_sf_ptr, _con);
  }
#endif

  /**
   * @brief For testing purposes. Might want to get rid of this.
   *
   * @return const container_t& A reference to the internal register container
   * object.
   */
  const container_t &container() { return _con; }

  //////////////////////////////////////////////////////////////////////////////
  // Insertion
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Insert item into registers using sketch functor.
   *
   * Example could involve `(x, multiplicity)` where `x` is the object to insert
   * and `multiplicity` is the number of insert repetitions to perform.
   *
   * @tparam ItemArgs types of parameters of the stream object to be inserted.
   * Will be used to construct an krowkee::stream::Element object.
   * @param args Arguments describing the stream object.
   */
  template <typename... ItemArgs>
  inline void insert(const ItemArgs &...args) {
    (*_sf_ptr)(_con, args...);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compactify
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Signal sketch container to compactify, if it supports such
   * functionality.
   *
   */
  void compactify() { _con.compactify(); }

  //////////////////////////////////////////////////////////////////////////////
  // Merge Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Merge other Sketch registers into `this`.
   *
   * @param rhs the other Sketch embedding. Must use the same random
   *     seed.
   * @return sk_t& The merge of the two sketches.
   * @throws std::invalid_argument if the `SketchFunc`s do not agree.
   */
  sk_t &operator+=(const sk_t &rhs) {
    if (!same_functors(rhs)) {
      std::stringstream ss;
      ss << "error: attempting to merge linear sketch objects with different "
            "hash functors : ("
         << *(_sf_ptr) << ") and (" << *(rhs._sf_ptr) << ")";
      throw std::invalid_argument(ss.str());
    }
    _con += rhs._con;
    return *this;
  }

  /**
   * @brief Merge two sketches
   *
   * @param lhs The left-hand sketch object.
   * @param rhs The right-hand sketch object.
   * @return sk_t The merge of the two sketch objects.
   */
  inline friend sk_t operator+(const sk_t &lhs, const sk_t &rhs) {
    sk_t ret(lhs);
    ret += rhs;
    return ret;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Get mutable begin iterator of container
   *
   * @return constexpr auto Iterator to the beginning of the sketch container.
   */
  constexpr auto begin() { return std::begin(_con); }
  /**
   * @brief Get const begin iterator of container
   *
   * @return constexpr auto const Iterator to the beginning of the sketch
   * container.
   */
  constexpr auto begin() const { return std::cbegin(_con); }
  /**
   * @brief Get const begin iterator of container
   *
   * @return constexpr auto const Iterator to the beginning of the sketch
   * container.
   */
  constexpr auto cbegin() const { return std::cbegin(_con); }
  /**
   * @brief Get mutable end iterator of container
   *
   * @return constexpr auto Iterator to the end of the sketch container.
   */
  constexpr auto end() { return std::end(_con); }
  /**
   * @brief Get const end iterator of container
   *
   * @return constexpr auto const Iterator to the end of the sketch container.
   */
  constexpr auto end() const { return std::cend(_con); }
  /**
   * @brief Get const end iterator of container
   *
   * @return constexpr auto const Iterator to the end of the sketch container.
   */
  constexpr auto cend() { return std::cend(_con); }

  // constexpr const std::uint8_t &operator[](const std::uint64_t index) const {
  //   return _con[index];
  // }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Return a description of the type of container and transform that
   * this sketch implements.
   *
   * @return std::string Sketch description, e.g. "Dense CountSketch"
   */
  static inline std::string name() {
    std::stringstream ss;
    ss << container_t::name() << " " << sf_t::name();
    return ss.str();
  }

  /**
   * @brief Return a description of the fully-qualified type of container and
   * transform that this sketch implements.
   *
   * @return std::string Sketch description, e.g. "Promotable using
   * std::map<unsigned int, int, std::less<unsigned int>,
   * std::allocator<std::pair<unsigned int const, int> > > CountSketch using
   * MulAddShift hashes and 4 byte registers"
   */
  static inline std::string full_name() {
    std::stringstream ss;
    ss << container_t::full_name() << " " << sf_t::full_name();
    return ss.str();
  }

  /**
   * @brief Check for sparsity of container.
   *
   * @return true The container is sparse.
   * @return false The container is not sparse.
   */
  constexpr bool is_sparse() const { return _con.is_sparse(); }

  /**
   * @brief Get the number of registers, real or notional, in the container.
   *
   * @return constexpr std::size_t The size of the container.
   */
  constexpr std::size_t size() const { return _con.size(); }

  /**
   * @brief Get the number of bytes used by each register
   *
   * @return constexpr std::size_t The size of `RegType`.
   */
  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  /**
   * @brief Get the number of possible range elements returned by the sketch
   * functor.
   *
   * @return constexpr std::size_t The size of the sketch functor range.
   */
  constexpr std::size_t range_size() const { return _sf_ptr->range_size(); }

  /**
   * @brief Get the compaction threshold of the container.
   *
   * @return constexpr std::size_t The compaction threshold.
   */
  constexpr std::size_t compaction_threshold() const {
    return _con.compaction_threshold();
  }

  /**
   * @brief Get a copy of the raw vector of registers in the container.
   *
   * @return std::vector<RegType> The register vector.
   */
  std::vector<RegType> register_vector() const {
    return _con.register_vector();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Checks whether another sketch is using the same functor.
   *
   * @param rhs The other sketch.
   * @return true The functors agree.
   * @return false The functors disagree.
   */
  constexpr bool same_functors(const sk_t &rhs) const {
    return *_sf_ptr == *rhs._sf_ptr;
  }

  /**
   * @brief Checks equality between sketch objects.
   *
   * Two sketches are equivalent if they are using the same sketch functor and
   * their registers hold the same values.
   *
   * @param lhs The left-hand sketch.
   * @param rhs The right-hand sketch.
   * @return true The sketches are equivalent.
   * @return false The sketches disagree.
   */
  friend constexpr bool operator==(const sk_t &lhs, const sk_t &rhs) {
    if (!lhs.same_functors(rhs)) {
      return false;
    }
    return lhs._con == rhs._con;
  }

  /**
   * @brief Check inequality between two sketch objects.
   *
   * @param lhs The left-hand sketch.
   * @param rhs The right-hand sketch.
   * @return true The sketches disagree.
   * @return false The sketches are equivalent.
   */
  friend constexpr bool operator!=(const sk_t &lhs, const sk_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Swap two sketch objects.
   *
   * @note For some reason, calling std::swap on the container here causes a "no
   * matching function compiler error". Peculiar.
   *
   * @param lhs The left-hand sketch.
   * @param rhs The right-hand sketch.
   */
  friend void swap(sk_t &lhs, sk_t &rhs) {
    std::swap(lhs._sf_ptr, rhs._sf_ptr);
    swap(lhs._con, rhs._con);
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
   * @brief Copy-and-swap assignment operator.
   *
   * @note
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   *
   * @param rhs The right-hand sketch.
   * @return sk_t& The new value of this sketch, swapped with `rhs`.
   */
  sk_t &operator=(sk_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Serialize a sketch to human-readable output stream.
   *
   * @note Intended for debugging only.
   *
   * @param os The output stream.
   * @param sk The sketch object.
   * @return std::ostream&
   */
  friend std::ostream &operator<<(std::ostream &os, const sk_t &sk) {
    os << sk._con;
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
  friend RetType accumulate(const sk_t &sk, const RetType init) {
    return accumulate(sk._con, init);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif