// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/hash.hpp>
// #include "xxhash.h"

#if __has_include(<cereal/types/base_class.hpp>)
#include <cereal/types/base_class.hpp>
#endif

#include <cstdint>
#include <random>

namespace krowkee {
namespace hash {
/**
 * CountSketch hash functor base class
 *
 * Right now only supports hashing to a power of 2 range and returns a pair of
 * register address and polarity hashes given an unsigned int input.
 */
struct CountSketchHashBase {
  using self_type = CountSketchHashBase;

  /**
   * @param range the desired range for the hash function.
   * @param seed the random seed controlling any randomness.
   */
  template <typename... Args>
  CountSketchHashBase(const std::uint64_t range,
                      const std::uint64_t seed = default_seed,
                      const Args &...args) {}

  CountSketchHashBase() {}

  virtual constexpr std::size_t size() const = 0;
  virtual constexpr std::size_t seed() const = 0;
  inline std::string            state() const {
    std::stringstream ss;
    ss << "size: " << size() << ", seed: " << seed();
    return ss.str();
  }

  void swap(self_type &rhs) {}

#if __has_include(<cereal/types/base_class.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {}
#endif

  friend void swap(self_type &lhs, self_type &rhs) { lhs.swap(rhs); }
};

/**
 * 2-universal CountSketch hash functor base class
 *
 * A fast implementation of CountSketch hash operators. Returns a pair
 * of hash values into the register space and {+1, -1}, respectively, each drawn
 * from a separate (ideally) 2-universal hash functor.
 */
template <typename HashType = MulAddShift>
struct CountSketchHash : public CountSketchHashBase {
  using hash_type = HashType;
  using base_type = CountSketchHashBase;
  using self_type = CountSketchHash<hash_type>;

 protected:
  hash_type _register_hash;
  hash_type _polarity_hash;

 public:
  /**
   * @param range the desired range for the hash function.
   * @param seed the random seed controlling any randomness.
   */
  template <typename... Args>
  CountSketchHash(const std::uint64_t range,
                  const std::uint64_t seed = default_seed, const Args &...args)
      : base_type(range, seed, args...),
        _register_hash(range, seed, args...),
        _polarity_hash(2, wang64(seed), args...) {}

  CountSketchHash() : base_type() {}

  /**
   * Compute the register and polarity hash of an input.
   *
   * @tparam OBJ the object to be hashed. Presently must fit into a 64-bit
   *     register.
   *
   * @param x the obejct to be hashed.
   */
  template <typename OBJ>
  constexpr std::pair<std::uint64_t, std::int32_t> operator()(
      const OBJ &x) const {
    return {_register_hash(x), _polarity_hash(x) == 1 ? 1 : -1};
  }

  /**
   * Print functor name.
   */
  static inline std::string name() { return "CountSketchHash"; }

  virtual constexpr std::size_t size() const override {
    return _register_hash.size();
  }
  virtual constexpr std::size_t seed() const override {
    return _register_hash.seed();
  }

#if __has_include(<cereal/types/base_class.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_register_hash);
    archive(_polarity_hash);
  }
#endif

  friend void swap(self_type &lhs, self_type &rhs) {
    std::swap(lhs._register_hash, rhs._register_hash);
    std::swap(lhs._polarity_hash, rhs._polarity_hash);
    lhs.swap(rhs);
  }
  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return lhs._register_hash == rhs._register_hash &&
           lhs._polarity_hash == lhs._polarity_hash;
  }

  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !operator==(lhs, rhs);
  }
};

std::ostream &operator<<(std::ostream &os, const CountSketchHashBase &func) {
  os << func.state();
  return os;
}

}  // namespace hash
}  // namespace krowkee
