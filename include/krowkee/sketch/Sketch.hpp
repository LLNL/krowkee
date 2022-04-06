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
 * General Sketch Chassis.
 *
 * Implements the container handling infrastructure of a linear sketch
 * defined by a given sketch functor projecting into a fixed-size set of
 * registers supporting an (element-wise) merge operator. The registers might be
 * Dense, Sparse, or variable, depending on the templated ContainerType.
 */
template <template <typename, typename...> class SketchFunc,
          template <typename, typename> class ContainerType,
          template <typename> class MergeOp, typename RegType,
          template <typename> class PtrType, typename... Args>
class Sketch {
 public:
  typedef RegType                                  reg_t;
  typedef ContainerType<RegType, MergeOp<RegType>> container_t;
  typedef SketchFunc<RegType, Args...>             sf_t;
  typedef PtrType<sf_t>                            sf_ptr_t;
  typedef Sketch<SketchFunc, ContainerType, MergeOp, RegType, PtrType, Args...>
      sk_t;

 private:
  sf_ptr_t    _sf_ptr;
  container_t _con;

 public:
  /**
   * Initialize register vector.
   *
   * @param sf_ptr std::shared_ptr pointing to the desired linear sketch
   *     functor.
   */
  Sketch(const sf_ptr_t &sf_ptr, const std::size_t compaction_threshold = 100,
         const std::size_t promotion_threshold = 4096)
      : _con(sf_ptr->range_size(), compaction_threshold, promotion_threshold),
        _sf_ptr(sf_ptr) {}

  /**
   * copy constructor
   *
   * @param rhs krowkee::sketch::Sketch to be copied.
   */
  Sketch(const sk_t &rhs) : _con(rhs._con), _sf_ptr(rhs._sf_ptr) {}

  /**
   * default constructor (only use for move constructor!)
   */
  // Sketch() : _con(), _sf_ptr(std::make_shared<sf_t>(1)) {}
  // Sketch() : _con(), _make_def_ptr(), _sf_ptr(_make_def_ptr(1)) {}
  // Sketch() : _con(), _sf_ptr(_make_default_ptr()) {
  //   std::cout << "GETS HERE!!!!!" << std::endl;
  // }
  // Sketch() { std::cout << "GETS HERE!!!!!" << std::endl; }
  Sketch() {}

  // sf_ptr_t _make_default_ptr() { return sf_ptr_t(); }

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
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_sf_ptr, _con);
  }
#endif

  // For testing purposes. Might want to get rid of this.
  const container_t &container() { return _con; }

  //////////////////////////////////////////////////////////////////////////////
  // Insertion
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Insert item into registers using sketch functor.
   *
   * @tparam ItemArgs... types of parameters of the stream object to be
   *     inserted. Will be used to construct an krowkee::stream::Element object.
   *
   * @param x the object to be inserted.
   * @param multiplicity the number of insert repetitions to perform. Can be
   *     negative. Default is `1`.
   */
  template <typename... ItemArgs>
  inline void insert(const ItemArgs &...args) {
    (*_sf_ptr)(_con, args...);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compactify
  //////////////////////////////////////////////////////////////////////////////

  void compactify() { _con.compactify(); }

  //////////////////////////////////////////////////////////////////////////////
  // Merge Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Merge other Sketch registers into `this`.
   *
   * @param other the other Sketch embedding. Must use the same random
   *     seed.
   *
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

  inline friend sk_t operator+(const sk_t &lhs, const sk_t &rhs) {
    sk_t ret(lhs);
    ret += rhs;
    return ret;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  constexpr auto begin() { return std::begin(_con); }
  constexpr auto begin() const { return std::cbegin(_con); }
  constexpr auto cbegin() const { return std::cbegin(_con); }
  constexpr auto end() { return std::end(_con); }
  constexpr auto end() const { return std::cend(_con); }
  constexpr auto cend() { return std::cend(_con); }

  // constexpr const std::uint8_t &operator[](const std::uint64_t index) const {
  //   return _con[index];
  // }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  static inline std::string name() {
    std::stringstream ss;
    ss << container_t::name() << " " << sf_t::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << container_t::full_name() << " " << sf_t::full_name();
    return ss.str();
  }

  constexpr bool is_sparse() const { return _con.is_sparse(); }

  constexpr std::size_t size() const { return _con.size(); }

  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  constexpr std::size_t range_size() const { return _sf_ptr->range_size(); }

  constexpr std::size_t compaction_threshold() const {
    return _con.compaction_threshold();
  }

  std::vector<RegType> register_vector() const {
    return _con.register_vector();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////
  constexpr bool same_functors(const sk_t &rhs) const {
    return *_sf_ptr == *rhs._sf_ptr;
  }

  friend constexpr bool operator==(const sk_t &lhs, const sk_t &rhs) {
    if (!lhs.same_functors(rhs)) {
      return false;
    }
    return lhs._con == rhs._con;
  }
  friend constexpr bool operator!=(const sk_t &lhs, const sk_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////
  /**
   * Swap boilerplate.
   *
   * For some reason, calling std::swap on the container here causes a "no
   * matching function compiler error". Peculiar.
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
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   */
  sk_t &operator=(sk_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////
  friend std::ostream &operator<<(std::ostream &os, const sk_t &sk) {
    os << sk._con;
    return os;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accumulation
  //////////////////////////////////////////////////////////////////////////////

  template <typename RetType>
  friend RetType accumulate(const sk_t &sk, const RetType init) {
    return accumulate(sk._con, init);
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif