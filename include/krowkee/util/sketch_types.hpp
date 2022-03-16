// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_UTIL_SKETCH_TYPES_HPP
#define _KROWKEE_UTIL_SKETCH_TYPES_HPP

#include <cstring>

namespace krowkee {
namespace util {
enum class sketch_type_t : std::uint8_t {
  cst,
  fwht,
  sparse_cst,
  promotable_cst
};

sketch_type_t get_sketch_type(char *arg) {
  if (strcmp(arg, "cst") == 0) {
    return sketch_type_t::cst;
  } else if (strcmp(arg, "sparse_cst") == 0) {
    return sketch_type_t::sparse_cst;
  } else if (strcmp(arg, "fwht") == 0) {
    return sketch_type_t::fwht;
  } else if (strcmp(arg, "promotable_cst") == 0) {
    return sketch_type_t::promotable_cst;
  } else {
    std::stringstream ss;
    ss << "error: requested sketch type " << arg << " is not supported!";
    throw std::invalid_argument(ss.str());
  }
}
}  // namespace util
}  // namespace krowkee

#endif