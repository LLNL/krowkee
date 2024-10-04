// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/stream/Element.hpp>

namespace krowkee {
namespace stream {

/**
 * Stream summary data to be held by a distributed map.
 *
 * This class holds a sketch `sk` and is meant to be the value type of a
 * distributed map. More sophisticated versions of stream data containing
 * additional information must support the same interface.
 */
template <typename SketchType, template <typename> class PtrType>
struct Summary {
  using sketch_type        = SketchType;
  using transform_type     = typename sketch_type::transform_type;
  using transform_ptr_type = typename sketch_type::transform_ptr_type;
  using register_type      = typename sketch_type::register_type;
  using self_type          = Summary<SketchType, PtrType>;

  sketch_type sk;

  Summary(const transform_ptr_type &ptr, const std::size_t compaction_threshold,
          const std::size_t promotion_threshold)
      : sk(ptr, compaction_threshold, promotion_threshold) {}

  template <typename... ItemArgs>
  Summary(const transform_ptr_type &ptr, const std::size_t compaction_threshold,
          const std::size_t promotion_threshold, const ItemArgs &...args)
      : sk(ptr, compaction_threshold, promotion_threshold) {
    update(args...);
  }

  Summary(const self_type &rhs) : sk(rhs.sk) {}
  Summary() : sk() {}

  template <class Archive>
  void serialize(Archive &archive) {
    archive(sk);
  }

  static inline std::string name() {
    std::stringstream ss;
    ss << "Summary using " << sketch_type::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Summary using " << sketch_type::full_name();
    return ss.str();
  }

  friend void swap(self_type &lhs, self_type &rhs) { swap(lhs.sk, rhs.sk); }

  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return lhs.sk == rhs.sk;
  }

  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !(lhs == rhs);
  }

  self_type &operator=(self_type rhs) {
    swap(*this, rhs);
    return *this;
  }

  self_type &operator+=(const self_type &rhs) {
    sk += rhs.sk;
    return *this;
  }

  inline friend self_type operator+(const self_type &lhs,
                                    const self_type &rhs) {
    self_type ret(lhs);
    ret += rhs;
    return ret;
  }

  template <typename... ItemArgs>
  void update(const ItemArgs &...args) {
    sk.insert(args...);
  }

  void compactify() { sk.compactify(); }

  friend std::ostream &operator<<(std::ostream &os, const self_type &data) {
    os << data.sk;
    return os;
  }
};

/**
 * Stream summary data to be held by a distributed map.
 *
 * This class holds a sketch `sk` and a counter `count` and is meant to be the
 * value type of a distributed map.
 */
template <typename SketchType, template <typename> class PtrType>
struct CountingSummary {
  using sketch_type        = SketchType;
  using transform_type     = typename sketch_type::transform_type;
  using transform_ptr_type = typename sketch_type::transform_ptr_type;
  using register_type      = typename sketch_type::register_type;
  using self_type          = CountingSummary<SketchType, PtrType>;

  sketch_type   sk;
  std::uint64_t count;
  CountingSummary(const transform_ptr_type &ptr,
                  const std::size_t         compaction_threshold,
                  const std::size_t         promotion_threshold)
      : sk(ptr, compaction_threshold, promotion_threshold), count(0) {}

  template <typename... ItemArgs>
  CountingSummary(const transform_ptr_type &ptr,
                  const std::size_t         compaction_threshold,
                  const std::size_t         promotion_threshold,
                  const ItemArgs &...args)
      : sk(ptr, compaction_threshold, promotion_threshold), count(0) {
    update(args...);
  }
  /// copy-and-swap boilerplate
  CountingSummary(const self_type &rhs) : sk(rhs.sk), count(rhs.count) {}
  CountingSummary() : sk() {}

  template <class Archive>
  void serialize(Archive &archive) {
    archive(sk, count);
  }

  static inline std::string name() {
    std::stringstream ss;
    ss << "Counting Summary using " << sketch_type::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Counting Summary using " << sketch_type::full_name();
    return ss.str();
  }

  /**
   * Swap boilerplate.
   *
   * For some reason, calling std::swap on the sketches here causes a
   * segfault. Peculiar.
   */
  friend void swap(self_type &lhs, self_type &rhs) {
    std::swap(lhs.count, rhs.count);
    swap(lhs.sk, rhs.sk);
  }

  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return lhs.count == rhs.count && lhs.sk == rhs.sk;
  }

  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !(lhs == rhs);
  }

  self_type &operator=(self_type rhs) {
    swap(*this, rhs);
    return *this;
  }

  self_type &operator+=(const self_type &rhs) {
    sk += rhs.sk;
    count += rhs.count;
    return *this;
  }

  inline friend self_type operator+(const self_type &lhs,
                                    const self_type &rhs) {
    self_type ret(lhs);
    ret += rhs;
    return ret;
  }

  /// interaction
  template <typename... ItemArgs>
  void update(const ItemArgs &...args) {
    // This is kind of klugy. Probably should re-engineer.
    sk.insert(args...);
    Element<register_type> element(args...);
    count += element.multiplicity;
  }

  void compactify() { sk.compactify(); }

  friend std::ostream &operator<<(std::ostream &os, const self_type &data) {
    os << data.sk;
    return os;
  }
};

}  // namespace stream
}  // namespace krowkee
