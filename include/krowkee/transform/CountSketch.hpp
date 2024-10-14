// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/stream/Element.hpp>

#include <krowkee/hash/util.hpp>

#include <sstream>
#include <vector>

namespace krowkee {
namespace transform {

using krowkee::stream::Element;

/// good reference for operator declarations:
/// https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading

/**
 * @brief A functor implementing CountSketch on a collection of registers.
 *
 * Implements CountSketch using a single pair of hash functions (i.e. no
 * Chernoff approximation tricks).
 *
 * [0] M. Charikar, K. Chen, M. Farach-Colton. Finding frequent items in data
 * streams. Theoretical Computer Science. 2004.
 * https://edoliberty.github.io/datamining2011aFiles/FindingFrequentItemsInDataStreams.pdf
 *
 * [1] K. Clarkson, D. Woodruff. Low-rank approximation in input sparsity time.
 * Journal of the ACM. 2017.
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.746.6409&rep=rep1&type=pdf
 *
 * @tparam RegType The type of register over which the functor operates
 * @tparam HashType The hash functor type to use to define CountSketch random
 * mappings.
 * @tparam RangeSize The power-of-two embedding dimension.
 */
template <typename RegType, typename HashType, std::size_t RangeSize>
class CountSketchFunctor {
 public:
  using register_type = RegType;
  using hash_type     = HashType;
  using self_type     = CountSketchFunctor<register_type, hash_type, RangeSize>;

 private:
  hash_type _hash;

 public:
  /**
   * @brief Construct a new Count Sketch Functor object by initializing hash
   * functors.
   *
   * Depending on the hash functor to be used, the effective embedding dimension
   * (returned by `this->size()`) may be rounded up to the next power of two.
   * Further, we are assuming no redundancy. Each insert is only hashed to one
   * register location.
   *
   * @note This behavior may change in the future.
   *
   * @tparam Args type(s) of additional hash parameters
   * @param seed The random seed.
   * @param args Any additional parameters required by the hash functions.
   */
  template <typename... Args>
  CountSketchFunctor(const std::uint64_t seed = krowkee::hash::default_seed,
                     const Args &...args)
      : _hash(RangeSize, seed, args...) {}

  CountSketchFunctor() {}

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/cereal.hpp>)
  /**
   * @brief Serialize CountSketchFunctor object to/from `cereal` archive.
   *
   * @tparam Archive `cereal` archive type.
   * @param archive The `cereal` archive to which to serialize the transform.
   */
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_hash);
  }
#endif

  //////////////////////////////////////////////////////////////////////////////
  // Function: Apply to Container
  //////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Update a register container with an observation.
   *
   * @tparam ContainerType The type of the register data structure.
   * @tparam MergeOp The merge operator used to combine registers.
   * @tparam ItemArgs Types of stream item observables.
   * @param registers The register container.
   * @param item_args Stream item observables.
   */
  template <template <typename, typename> class ContainerType, typename MergeOp,
            typename... ItemArgs>
  constexpr void operator()(ContainerType<RegType, MergeOp> &registers,
                            const ItemArgs &...item_args) const {
    _apply_to_container<MergeOp>(registers, item_args...);
  }

  /**
   * @brief Update a register container with an observation.
   *
   * Overload to prior operator for sparse containers.
   *
   * @tparam ContainerType The type of the register data structure.
   * @tparam MergeOp The merge operator used to combine registers.
   * @tparam MapType The compacting map type used by the container.
   * @tparam KeyType The key type used by the compacting map.
   * @tparam ItemArgs Types of stream item observables.
   * @param registers The register container.
   * @param item_args Stream item observables.
   */
  template <template <typename, typename, template <typename, typename> class,
                      typename>
            class ContainerType,
            typename MergeOp, template <typename, typename> class MapType,
            typename KeyType, typename... ItemArgs>
  constexpr void operator()(
      ContainerType<register_type, MergeOp, MapType, KeyType> &registers,
      const ItemArgs &...item_args) const {
    _apply_to_container<MergeOp>(registers, item_args...);
  }

 private:
  template <typename MergeOp, typename ContainerType, typename... ItemArgs>
  constexpr void _apply_to_container(ContainerType &registers,
                                     const ItemArgs &...item_args) const {
    const Element<register_type> stream_element(item_args...);
    const auto [index, polarity] = _hash(stream_element.item);
    register_type &reg           = registers[index];
    reg = MergeOp()(reg, polarity * stream_element.multiplicity);
    if (reg == 0) {
      registers.erase(index);
    }
  }

 public:
  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////
  /**
   * @brief Get the maximum number of range values returnable by the outer hash
   * function.
   *
   * This is equivalent to the maximum number of elements in passed containers.
   *
   * @return constexpr std::size_t The range size.
   */
  constexpr std::size_t range_size() const { return _hash.size(); }

  /** Get the random seed. */
  constexpr std::uint64_t seed() const { return _hash.seed(); }

  /**
   * @brief Return a description of the transform type.
   *
   * @return std::string "CountSketch"
   */
  static inline std::string name() { return "CountSketch"; }

  /**
   * @brief Return a description of the fully-qualified transform type.
   *
   * @return std::string Transform description, e.g. "CountSketch using
   * MulAddShift hashes and 4 byte registers"
   */
  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << HashType::name() << " hashes and " << RangeSize
       << " " << sizeof(register_type) << "-byte registers";
    return ss.str();
  }

  /**
   * @brief Check for equality between two CountSketchFunctors.
   *
   * @param lhs The left-hand functor.
   * @param rhs The right-hand functor.
   * @return true The seeds and range sizes agree.
   * @return false The seeds or range sizes disagree.
   */
  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return (lhs.seed() == rhs.seed()) && (lhs.range_size() == rhs.range_size());
  }

  /**
   * @brief Check for inequality between two CountSketchFunctors.
   *
   * @param lhs The left-hand functor.
   * @param rhs The right-hand functor.
   * @return true The seeds or range sizes disagree.
   * @return false The seeds and range sizes agree.
   */
  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !operator==(lhs, rhs);
  }

  /**
   * @brief Serialize a transform to human-readable output stream.
   *
   * Prints the space-separated range size and seed.
   *
   * @param os The output stream.
   * @param func The functor object.
   * @return std::ostream& The new stream state.
   */
  friend std::ostream &operator<<(std::ostream &os, const self_type &func) {
    os << func.range_size() << " " << func.seed();
    return os;
  }
};

}  // namespace transform
}  // namespace krowkee
