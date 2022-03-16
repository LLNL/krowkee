// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_UTIL_CMAP_TYPES_HPP
#define _KROWKEE_UTIL_CMAP_TYPES_HPP

#include <cstring>

namespace krowkee {
namespace util {
enum class cmap_type_t : std::uint8_t { std, boost };

cmap_type_t get_cmap_type(char *arg) {
  if (strcmp(arg, "std") == 0) {
    return cmap_type_t::std;
#if __has_include(<boost/container/flat_map.hpp>)
  } else if (strcmp(arg, "boost") == 0) {
    return cmap_type_t::boost;
#endif
  } else {
    std::stringstream ss;
    ss << "error: requested map type " << arg << " is not supported !";
    throw std::invalid_argument(ss.str());
  }
}
}  // namespace util
}  // namespace krowkee

#endif