// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_TRANSFORM_FWHT_HPP
#define _KROWKEE_TRANSFORM_FWHT_HPP

#include <krowkee/transform/fwht/utils.hpp>

#include <krowkee/stream/Element.hpp>

#include <sstream>
#include <vector>

namespace krowkee {
namespace transform {

using krowkee::stream::Element;

/// good reference for operator declarations:
/// https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading

/**
 * Fast Walsh-Hadamard Transform Functor Class
 */
template <typename RegType>
class FWHTFunctor {
  typedef FWHTFunctor<RegType> fwhtf_t;

 protected:
  std::uint64_t _range_size;
  std::uint64_t _seed;
  std::uint64_t _domain_size;

 public:
  /**Quick
   * Initialize members.
   *
   * @note[BWP] Do we actually need to pass n?
   *
   * @tparam ContainerType The type of the underlying sketch data structure.
   * @tparam Args type(s) of additional hash parameters.
   *
   * @param s the desired embedding dimension.
   * @param seed the random seed.
   * @param args any additional paramters required by the hash functions.
   */
  template <typename... Args>
  FWHTFunctor<RegType>(const std::uint64_t range_size = 64,
                       const std::uint64_t seed = krowkee::hash::default_seed,
                       const std::uint64_t domain_size = 1024,
                       const Args &&...args)
      : _range_size(range_size), _seed(seed), _domain_size(domain_size) {}

  FWHTFunctor() {}

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/cereal.hpp>)
  template <class Archive>
  void serialize(Archive &archive) {
    archive(_range_size, _seed, _domain_size);
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
   *     inserted. Will be used to construct an krowkee::stream::Element object.
   *
   * @param[out] registers the vector of registers.
   * @param[in] x the object to be inserted.
   * @param[in] multiplicity a multiple to modulate insertion.
   */
  template <template <typename, typename> class ContainerType, typename MergeOp,
            typename... ItemArgs>
  constexpr void operator()(ContainerType<RegType, MergeOp> &registers,
                            const ItemArgs &...item_args) const {
    const Element<RegType> stream_element(item_args...);
    const std::uint64_t    col_index    = stream_element.item;
    const std::uint64_t    row_index    = stream_element.identifier;
    const RegType          multiplicity = stream_element.multiplicity;
    std::vector<RegType>   sketch_vec =
        krowkee::transform::fwht::get_sketch_vector(multiplicity, row_index,
                                                    col_index, _domain_size,
                                                    _range_size, _seed);

    std::transform(std::begin(registers), std::end(registers),
                   std::begin(sketch_vec), std::begin(registers),
                   std::plus<RegType>());
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  constexpr std::size_t range_size() const { return _range_size; }

  constexpr std::uint64_t seed() const { return _seed; }

  constexpr std::uint64_t domain_size() const { return _domain_size; }

  static inline std::string name() { return "FWHT"; }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << sizeof(RegType) << " byte registers";
    return ss.str();
  }
};

template <typename RegType>
constexpr bool operator==(const FWHTFunctor<RegType> &lhs,
                          const FWHTFunctor<RegType> &rhs) {
  return lhs.seed() == rhs.seed() && lhs.range_size() == rhs.range_size();
}

template <typename RegType>
constexpr bool operator!=(const FWHTFunctor<RegType> &lhs,
                          const FWHTFunctor<RegType> &rhs) {
  return !operator==(lhs, rhs);
}

template <typename RegType>
std::ostream &operator<<(std::ostream &os, const FWHTFunctor<RegType> &func) {
  os << func.range_size() << " " << func.domain_size() << " " << func.seed();
  return os;
}
}  // namespace transform
}  // namespace krowkee

#endif