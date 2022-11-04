// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/hash.hpp>
#include <krowkee/sketch/interface.hpp>

using Dense32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::Dense, std::int32_t>;
using MapSparse32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::MapSparse32,
                                      std::int32_t>;
using MapPromotable32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::MapPromotable32,
                                      std::int32_t>;
#if __has_include(<boost/container/flat_map.hpp>)
using FlatMapSparse32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::FlatMapSparse32,
                                      std::int32_t>;

using FlatMapPromotable32CountSketch =
    krowkee::sketch::LocalCountSketch<krowkee::sketch::FlatMapPromotable32,
                                      std::int32_t>;
#endif
