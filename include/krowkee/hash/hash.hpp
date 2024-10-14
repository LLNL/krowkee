// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/util.hpp>
// #include "xxhash.h"

#if __has_include(<cereal/types/base_class.hpp>)
#include <cereal/types/base_class.hpp>
#endif

#include <cstdint>
#include <random>

namespace krowkee {
namespace hash {
/**
 * Hash functor base class
 *
 * Right now only supports hashing to a power of 2 range.
 *
 * @tparam Range The desired range for the hash function. Gets rounded up to the
 * nearest power of 2.
 */
struct Base {
  /**
   * @param range the desired range for the hash function. Currently we round up
   * to the nearest power of 2 for `_range'.
   * @param seed the random seed controlling any randomness. Might be ignored.
   */
  Base(const std::uint64_t range, const std::uint64_t seed = default_seed)
      : _range(64 - ceil_log2_64(range)), _seed(seed) {}

  Base() {}

  /**
   * Truncate a hashed 64-bit value to the desired range specified by `_range`.
   *
   * @param val the hashed value to be truncated.
   */
  constexpr std::uint64_t truncate(const std::uint64_t val) const {
    return val >> _range;
  }

  constexpr std::uint64_t get_range() const { return _range; }
  constexpr std::size_t   size() const { return _1u64 << (64 - _range); }
  constexpr std::size_t   seed() const { return _seed; }
  inline std::string      state() const {
    std::stringstream ss;
    ss << "size: " << size() << ", seed: " << seed();
    return ss.str();
  }

  void swap(Base &rhs) {
    std::swap(_range, rhs._range);
    std::swap(_seed, rhs._seed);
  }

#if __has_include(<cereal/types/base_class.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_range, _seed);
  }
#endif

  friend void swap(Base &lhs, Base &rhs) { lhs.swap(rhs); }

 protected:
  std::uint64_t              _range;
  std::uint64_t              _seed;
  const static std::uint64_t _1u64 = 1;
};

/**
 * Thomas Wang hash functor
 *
 * https://naml.us/blog/tag/thomas-wang
 */
struct WangHash : public Base {
  /**
   * @tparam ARGS types of additional (ignored) hash functor arguments.
   * @param range the power of 2 range for the hash function.
   */
  template <typename... ARGS>
  WangHash(std::uint64_t range, ARGS &...) : Base(range) {}

  WangHash() : Base() {}

  template <typename OBJ>
  constexpr std::uint64_t operator()(const OBJ &x) const {
    return truncate(wang64(std::uint64_t(x)));
  }

  /**
   * Print functor name.
   */
  static inline std::string name() { return "WangHash"; }

  inline std::string state() const {
    std::stringstream ss;
    ss << "size: " << size();
    return ss.str();
  }

  friend constexpr bool operator==(const WangHash &lhs, const WangHash &rhs) {
    return lhs._range == rhs._range && lhs._seed == rhs._seed;
  }

  friend constexpr bool operator!=(const WangHash &lhs, const WangHash &rhs) {
    return !operator==(lhs, rhs);
  }
};

// /**
//  * XXHash functor
//  *
//  * https://github.com/Cyan4973/xxHash
//  */
// struct XXHash : public Base {
//   template <typename... ARGS>
//   XXHash(const std::uint64_t m, const std::uint64_t seed = default_seed,
//          const ARGS &&...)
//       : Base(m, std::uint32_t(seed)) {}

//   template <typename OBJ>
//   constexpr std::uint64_t operator()(const OBJ &x) const {
//     std::cout << "gets here" << std::endl;
//     return xxh::xxhash<64>(&x, 1, _seed);
//     // return std::uint64_t(XXH64(&x, sizeof(x), _seed));
//   }

//   inline std::string name() { return "XXHash"; }
// };

/**
 * multiply-shift hash
 *
 * https://en.wikipedia.org/wiki/Universal_hashing
 */
struct MulShift : public Base {
  /**
   * Initialize `_multiplicand` given the specified random `seed`.
   *
   * `_multiplicand` is randomly sampled from [0, 2^64 - 1].
   *
   * @tparam ARGS types of additional (ignored) hash functor arguments.
   *
   * @param range the range for the hash function.
   * @param seed the random seed.
   */
  template <typename... ARGS>
  MulShift(const std::uint64_t range, const std::uint64_t seed = default_seed,
           const ARGS &...)
      : Base(range, seed) {
    std::mt19937_64                              rnd_gen(wang64(_seed));
    std::uniform_int_distribution<std::uint64_t> udist(
        1, std::numeric_limits<std::uint64_t>::max());
    std::uint64_t multiplicand;
    do {
      multiplicand = udist(rnd_gen);
    } while (is_even(multiplicand));
    _multiplicand = multiplicand;
  }

  MulShift() : Base() {}

  /**
   * Compute `_multiplicand * x mod _range`.
   *
   * @tparam OBJ the object to be hashed. Presently must fit into a 64-bit
   *     register.
   *
   * @param x the obejct to be hashed.
   */
  template <typename OBJ>
  constexpr std::uint64_t operator()(const OBJ &x) const {
    return truncate(_multiplicand * x);
  }

  /**
   * Print functor name.
   */
  static inline std::string name() { return "MulShift"; }

  inline std::string state() const {
    std::stringstream ss;
    ss << Base::state() << ", multiplicand: " << _multiplicand;
    return ss.str();
  }

  friend void swap(MulShift &lhs, MulShift &rhs) {
    std::swap(lhs._multiplicand, rhs._multiplicand);
    lhs.swap(rhs);
  }

  friend constexpr bool operator==(const MulShift &lhs, const MulShift &rhs) {
    return lhs._range == rhs._range && lhs._seed == rhs._seed &&
           lhs._multiplicand == rhs._multiplicand;
  }

  friend constexpr bool operator!=(const MulShift &lhs, const MulShift &rhs) {
    return !operator==(lhs, rhs);
  }

#if __has_include(<cereal/types/base_class.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(cereal::base_class<Base>(this), _multiplicand);
  }
#endif

 private:
  std::uint64_t _multiplicand;
};

/**
 * multiply-add-shift hash
 *
 * https://en.wikipedia.org/wiki/Universal_hashing
 */
struct MulAddShift : public Base {
  /**
   * Initialize `_multiplicand` and `_summand` given the specified random
   * `seed`.
   *
   * `_multiplicand` and `_summand` are randomly sampled from [0, 2^64 - 1] and
   * [0, 2^size() - 1], respectively.
   *
   * @tparam ARGS types of additional (ignored) hash functor arguments.
   *
   * @param range the power of 2 range for the hash function.
   * @param seed the random seed.
   */
  template <typename... ARGS>
  MulAddShift(const std::uint64_t range,
              const std::uint64_t seed = default_seed, const ARGS &...)
      : Base(range, seed) {
    std::mt19937_64                              rnd_gen(wang64(_seed));
    std::uniform_int_distribution<std::uint64_t> udist_multiplicand(
        0, std::numeric_limits<std::uint64_t>::max());
    std::uniform_int_distribution<std::uint64_t> udist_summand(0, size());
    std::uint64_t                                multiplicand;
    do {
      multiplicand = udist_multiplicand(rnd_gen);
    } while (is_even(multiplicand));
    _multiplicand = multiplicand;
    _summand      = udist_summand(rnd_gen);
  }

  MulAddShift() : Base() {}

  /**
   * Compute `_multiplicand * x + _summand mod _range`.
   *
   * @tparam OBJ the object to be hashed. Presently must fit into a 64-bit
   *     register.
   *
   * @param x the obejct to be hashed.
   */
  template <typename OBJ>
  constexpr std::uint64_t operator()(const OBJ &x) const {
    return truncate(_multiplicand * x + _summand);
  }

  /**
   * Print functor name.
   */
  static inline std::string name() { return "MulAddShift"; }

  inline std::string state() const {
    std::stringstream ss;
    ss << Base::state() << ", multiplicand: " << _multiplicand
       << ",  summand: " << _summand;
    return ss.str();
  }

  friend void swap(MulAddShift &lhs, MulAddShift &rhs) {
    std::swap(lhs._multiplicand, rhs._multiplicand);
    std::swap(lhs._summand, rhs._summand);
    lhs.swap(rhs);
  }

  friend constexpr bool operator==(const MulAddShift &lhs,
                                   const MulAddShift &rhs) {
    return lhs._range == rhs._range && lhs._seed == rhs._seed &&
           lhs._multiplicand == rhs._multiplicand &&
           lhs._summand == rhs._summand;
  }

  friend constexpr bool operator!=(const MulAddShift &lhs,
                                   const MulAddShift &rhs) {
    return !operator==(lhs, rhs);
  }

#if __has_include(<cereal/types/base_class.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(cereal::base_class<Base>(this), _multiplicand, _summand);
  }
#endif

 private:
  std::uint64_t _multiplicand;
  std::uint64_t _summand;
};

std::ostream &operator<<(std::ostream &os, const Base &func) {
  os << func.state();
  return os;
}
}  // namespace hash
}  // namespace krowkee
