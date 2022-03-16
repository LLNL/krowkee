// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_STREAM_SUMMARY_HPP
#define _KROWKEE_STREAM_SUMMARY_HPP

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
  typedef SketchType                   sk_t;
  typedef typename sk_t::sf_t          sf_t;
  typedef typename sk_t::sf_ptr_t      sf_ptr_t;
  typedef typename sk_t::reg_t         reg_t;
  typedef Summary<SketchType, PtrType> data_t;

  sk_t sk;

  Summary(const sf_ptr_t &ptr, const std::size_t compaction_threshold,
          const std::size_t promotion_threshold)
      : sk(ptr, compaction_threshold, promotion_threshold) {}

  template <typename... ItemArgs>
  Summary(const sf_ptr_t &ptr, const std::size_t compaction_threshold,
          const std::size_t promotion_threshold, const ItemArgs &...args)
      : sk(ptr, compaction_threshold, promotion_threshold) {
    update(args...);
  }

  Summary(const data_t &rhs) : sk(rhs.sk) {}
  Summary() : sk() {}

  template <class Archive>
  void serialize(Archive &archive) {
    archive(sk);
  }

  static inline std::string name() {
    std::stringstream ss;
    ss << "Summary using " << sk_t::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Summary using " << sk_t::full_name();
    return ss.str();
  }

  friend void swap(data_t &lhs, data_t &rhs) { swap(lhs.sk, rhs.sk); }

  friend constexpr bool operator==(const data_t &lhs, const data_t &rhs) {
    return lhs.sk == rhs.sk;
  }

  friend constexpr bool operator!=(const data_t &lhs, const data_t &rhs) {
    return !(lhs == rhs);
  }

  data_t &operator=(data_t rhs) {
    swap(*this, rhs);
    return *this;
  }

  data_t &operator+=(const data_t &rhs) {
    sk += rhs.sk;
    return *this;
  }

  inline friend data_t operator+(const data_t &lhs, const data_t &rhs) {
    data_t ret(lhs);
    ret += rhs;
    return ret;
  }

  template <typename... ItemArgs>
  void update(const ItemArgs &...args) {
    sk.insert(args...);
  }

  void compactify() { sk.compactify(); }

  friend std::ostream &operator<<(std::ostream &os, const data_t &data) {
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
  typedef SketchType                           sk_t;
  typedef typename sk_t::sf_t                  sf_t;
  typedef typename sk_t::sf_ptr_t              sf_ptr_t;
  typedef typename sk_t::reg_t                 reg_t;
  typedef CountingSummary<SketchType, PtrType> data_t;

  sk_t          sk;
  std::uint64_t count;
  CountingSummary(const sf_ptr_t &ptr, const std::size_t compaction_threshold,
                  const std::size_t promotion_threshold)
      : sk(ptr, compaction_threshold, promotion_threshold), count(0) {}

  template <typename... ItemArgs>
  CountingSummary(const sf_ptr_t &ptr, const std::size_t compaction_threshold,
                  const std::size_t promotion_threshold,
                  const ItemArgs &...args)
      : sk(ptr, compaction_threshold, promotion_threshold), count(0) {
    update(args...);
  }
  /// copy-and-swap boilerplate
  CountingSummary(const data_t &rhs) : sk(rhs.sk), count(rhs.count) {}
  CountingSummary() : sk() {}

  template <class Archive>
  void serialize(Archive &archive) {
    archive(sk, count);
  }

  static inline std::string name() {
    std::stringstream ss;
    ss << "Counting Summary using " << sk_t::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Counting Summary using " << sk_t::full_name();
    return ss.str();
  }

  /**
   * Swap boilerplate.
   *
   * For some reason, calling std::swap on the sketches here causes a
   * segfault. Peculiar.
   */
  friend void swap(data_t &lhs, data_t &rhs) {
    std::swap(lhs.count, rhs.count);
    swap(lhs.sk, rhs.sk);
  }

  friend constexpr bool operator==(const data_t &lhs, const data_t &rhs) {
    return lhs.count == rhs.count && lhs.sk == rhs.sk;
  }

  friend constexpr bool operator!=(const data_t &lhs, const data_t &rhs) {
    return !(lhs == rhs);
  }

  data_t &operator=(data_t rhs) {
    swap(*this, rhs);
    return *this;
  }

  data_t &operator+=(const data_t &rhs) {
    sk += rhs.sk;
    count += rhs.count;
    return *this;
  }

  inline friend data_t operator+(const data_t &lhs, const data_t &rhs) {
    data_t ret(lhs);
    ret += rhs;
    return ret;
  }

  /// interaction
  template <typename... ItemArgs>
  void update(const ItemArgs &...args) {
    // This is kind of klugy. Probably should re-engineer.
    sk.insert(args...);
    Element<reg_t> element(args...);
    count += element.multiplicity;
  }

  void compactify() { sk.compactify(); }

  friend std::ostream &operator<<(std::ostream &os, const data_t &data) {
    os << data.sk;
    return os;
  }
};

}  // namespace stream
}  // namespace krowkee

#endif