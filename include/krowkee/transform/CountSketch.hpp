// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_TRANSFORM_COUNTSKETCH_HPP
#define _KROWKEE_TRANSFORM_COUNTSKETCH_HPP

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
 * CountSketchFunctor
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
 */
template <typename RegType, typename HashFunc>
class CountSketchFunctor {
  typedef CountSketchFunctor<RegType, HashFunc> csf_t;
  HashFunc                                      _reg_hf;
  HashFunc                                      _pol_hf;

 public:
  /**
   * Initialize hash functors.
   *
   * Depending on the hash functor to be used, the effective embedding dimension
   * (returned by `this->size()`) may be rounded up to the next power of two.
   * Further, we are assuming no redundancy. Each insert is only hashed to one
   * register location. This behavior may change in the future.
   *
   * @tparam Args type(s) of additional hash parameters.
   *
   * @param s the desired embedding dimension.
   * @param seed the random seed.
   * @param args any additional paramters required by the hash functions.
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
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_reg_hf, _pol_hf);
  }
#endif

  //////////////////////////////////////////////////////////////////////////////
  // Function: Apply to Container
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Update a vector of registers with an observation.
   *
   * @tparam ContainerType The type of the underlying sketch data structure.
   * @tparam ItemArgs... types of parameters of the stream object to be
   *     inserted. Will be used to construct a krowkee::stream::Element object.
   *
   * @param[out] registers the vector of registers.
   * @param[in] x the object to be inserted.
   * @param[in] multiplicity a multiple to modulate insertion.
   */
  template <template <typename, typename> class ContainerType, typename MergeOp,
            typename... ItemArgs>
  constexpr void operator()(ContainerType<RegType, MergeOp> &registers,
                            const ItemArgs &...item_args) const {
    _apply_to_container<MergeOp>(registers, item_args...);
  }

  /**
   * Update a vector of registers with an observation.
   *
   * @tparam ContainerType The type of the underlying sketch data structure.
   * @tparam ItemArgs... types of parameters of the stream object to be
   *     inserted. Will be used to construct an krowkee::stream::Element object.
   *
   * @param[out] registers the vector of registers.
   * @param[in] x the object to be inserted.
   * @param[in] multiplicity a multiple to modulate insertion.
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

  constexpr std::size_t range_size() const { return _reg_hf.size(); }

  constexpr std::uint64_t seed() const { return _reg_hf.seed(); }

  static inline std::string name() { return "CountSketch"; }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << HashFunc::name() << " hashes and "
       << sizeof(RegType) << " byte registers";
    return ss.str();
  }
};

template <typename RegType, typename HashFunc>
constexpr bool operator==(const CountSketchFunctor<RegType, HashFunc> &lhs,
                          const CountSketchFunctor<RegType, HashFunc> &rhs) {
  return (lhs.seed() == rhs.seed()) && (lhs.range_size() == rhs.range_size());
}

template <typename RegType, typename HashFunc>
constexpr bool operator!=(const CountSketchFunctor<RegType, HashFunc> &lhs,
                          const CountSketchFunctor<RegType, HashFunc> &rhs) {
  return !operator==(lhs, rhs);
}

template <typename RegType, typename HashFunc>
std::ostream &operator<<(std::ostream                                &os,
                         const CountSketchFunctor<RegType, HashFunc> &func) {
  os << func.range_size() << " " << func.seed();
  return os;
}
}  // namespace transform
}  // namespace krowkee

#endif