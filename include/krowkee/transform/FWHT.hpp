// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

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
template <typename RegType, std::size_t RangeSize>
class FWHTFunctor {
 public:
  using register_type = RegType;
  using self_type     = FWHTFunctor<RegType, RangeSize>;

 protected:
  std::uint64_t _range_size = RangeSize;
  std::uint64_t _seed;
  std::uint64_t _domain_size;

 public:
  /**Quick
   * Initialize members.
   *
   * @note[BWP] Do we actually need to pass n?
   *
   * @tparam Args type(s) of additional hash parameters.
   *
   * @param seed the random seed.
   * @param args any additional paramters required by the hash functions.
   */
  template <typename... Args>
  FWHTFunctor(const std::uint64_t seed, const std::uint64_t domain_size = 1024,
              const Args &&...args)
      : _seed(seed), _domain_size(domain_size) {}

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

  static constexpr std::size_t range_size() { return RangeSize; }

  static constexpr std::size_t size() { return self_type::range_size(); }

  static constexpr double scaling_factor() {
    return std::sqrt((double)range_size());
  }

  constexpr std::uint64_t seed() const { return _seed; }

  constexpr std::uint64_t domain_size() const { return _domain_size; }

  static constexpr std::string name() { return "FWHT"; }

  static constexpr std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << RangeSize << " " << sizeof(RegType)
       << "-byte registers";
    return ss.str();
  }

  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return lhs.seed() == rhs.seed() && lhs.range_size() == rhs.range_size();
  }

  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !operator==(lhs, rhs);
  }

  friend std::ostream &operator<<(std::ostream &os, const self_type &func) {
    os << func.range_size() << " " << func.domain_size() << " " << func.seed();
    return os;
  }
};

}  // namespace transform
}  // namespace krowkee
