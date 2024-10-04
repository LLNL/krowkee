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
 * @tparam HashFunc The hash functor type to use to define CountSketch random
 * mappings.
 */
template <typename RegType, typename HashFunc>
class CountSketchFunctor {
  using self_type = CountSketchFunctor<RegType, HashFunc>;
  HashFunc _reg_hf;
  HashFunc _pol_hf;

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
   * @param range_size The desired embedding dimension.
   * @param seed The random seed.
   * @param args and additional parameters required by the hash functions.
   */
  template <typename... Args>
  CountSketchFunctor(const std::uint64_t range_size,
                     const std::uint64_t seed = krowkee::hash::default_seed,
                     const Args &...args)
      : _reg_hf(range_size, seed, args...),
        _pol_hf(2, krowkee::hash::wang64(seed), args...) {}

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
    archive(_reg_hf, _pol_hf);
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
      ContainerType<RegType, MergeOp, MapType, KeyType> &registers,
      const ItemArgs &...item_args) const {
    _apply_to_container<MergeOp>(registers, item_args...);
  }

 private:
  template <typename MergeOp, typename ContainerType, typename... ItemArgs>
  constexpr void _apply_to_container(ContainerType &registers,
                                     const ItemArgs &...item_args) const {
    const Element<RegType> stream_element(item_args...);
    const std::uint64_t    index(_reg_hf(stream_element.item));
    const RegType polarity((_pol_hf(stream_element.item) == 1) ? RegType(1)
                                                               : RegType(-1));
    RegType      &reg = registers[index];
    reg               = MergeOp()(reg, polarity * stream_element.multiplicity);
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
  constexpr std::size_t range_size() const { return _reg_hf.size(); }

  /** Get the random seed. */
  constexpr std::uint64_t seed() const { return _reg_hf.seed(); }

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
    ss << name() << " using " << HashFunc::name() << " hashes and "
       << sizeof(RegType) << " byte registers";
    return ss.str();
  }
};

/**
 * @brief Check for equality between two CountSketchFunctors.
 *
 * @tparam RegType The register type.
 * @tparam HashFunc The hash functor type.
 * @param lhs The left-hand functor.
 * @param rhs The right-hand functor.
 * @return true The seeds and range sizes agree.
 * @return false The seeds or range sizes disagree.
 */
template <typename RegType, typename HashFunc>
constexpr bool operator==(const CountSketchFunctor<RegType, HashFunc> &lhs,
                          const CountSketchFunctor<RegType, HashFunc> &rhs) {
  return (lhs.seed() == rhs.seed()) && (lhs.range_size() == rhs.range_size());
}

/**
 * @brief Check for inequality between two CountSketchFunctors.
 *
 * @tparam RegType The register type.
 * @tparam HashFunc The hash functor type.
 * @param lhs The left-hand functor.
 * @param rhs The right-hand functor.
 * @return true The seeds or range sizes disagree.
 * @return false The seeds and range sizes agree.
 */
template <typename RegType, typename HashFunc>
constexpr bool operator!=(const CountSketchFunctor<RegType, HashFunc> &lhs,
                          const CountSketchFunctor<RegType, HashFunc> &rhs) {
  return !operator==(lhs, rhs);
}

/**
 * @brief Serialize a transform to human-readable output stream.
 *
 * Prints the space-separated range size and seed.
 *
 * @tparam RegType The register type.
 * @tparam HashFunc The hash functor type.
 * @param os The output stream.
 * @param func The functor object.
 * @return std::ostream& The new stream state.
 */
template <typename RegType, typename HashFunc>
std::ostream &operator<<(std::ostream                                &os,
                         const CountSketchFunctor<RegType, HashFunc> &func) {
  os << func.range_size() << " " << func.seed();
  return os;
}
}  // namespace transform
}  // namespace krowkee
