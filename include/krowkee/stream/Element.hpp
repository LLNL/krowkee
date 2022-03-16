// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_STREAM_ELEMENT_HPP
#define _KROWKEE_STREAM_ELEMENT_HPP

#include <algorithm>
#include <sstream>
#include <vector>
// // #include <memory>

namespace krowkee {
namespace stream {

template <typename RegType>
struct Element {
  std::uint64_t item;
  std::uint64_t identifier;
  RegType       multiplicity;

  // Constructors from raw values, indices, and multiplicities
  Element(const std::uint64_t x) : item(x), identifier(0), multiplicity(1) {}
  Element(const std::uint64_t x, std::uint64_t idx)
      : item(x), identifier(idx), multiplicity(1) {}
  Element(const std::uint64_t x, RegType mult)
      : item(x), identifier(0), multiplicity(mult) {}
  Element(const std::uint64_t x, const std::uint64_t idx, RegType mult)
      : item(x), identifier(idx), multiplicity(mult) {}

  // Copy constructor
  Element(const Element<RegType> &rhs)
      : item(rhs.item),
        identifier(rhs.identifier),
        multiplicity(rhs.multiplicity) {}
};

template <typename RegType>
std::ostream &operator<<(std::ostream &os, const Element<RegType> &item) {
  os << item.item << " " << item.identifier << " "
     << std::int64_t(item.multiplicity);
  return os;
}

}  // namespace stream
}  // namespace krowkee

#endif