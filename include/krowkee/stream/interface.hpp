// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/countsketch.hpp>

#include <krowkee/transform/CountSketch.hpp>
#include <krowkee/transform/FWHT.hpp>

#include <krowkee/sketch/Dense.hpp>
#include <krowkee/sketch/Promotable.hpp>
#include <krowkee/sketch/Sparse.hpp>

#include <krowkee/sketch/Sketch.hpp>

#include <krowkee/stream/Multi.hpp>
#include <krowkee/stream/Summary.hpp>

#include <map>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// Multi Stream Type Presets
////////////////////////////////////////////////////////////////////////////////

namespace krowkee {
namespace stream {

template <typename SketchFunc,
          template <typename, typename> class ContainerType,
          template <typename> class MergeOp, typename KeyType, typename RegType>
using MultiLocal =
    Multi<CountingSummary, krowkee::sketch::Sketch, SketchFunc, ContainerType,
          MergeOp, KeyType, RegType, std::shared_ptr>;

template <template <typename, typename> class ContainerType, typename KeyType,
          typename RegType, std::size_t RangeSize>
using MultiLocalCountSketch =
    MultiLocal<krowkee::transform::CountSketchFunctor<
                   RegType, krowkee::hash::CountSketchHash, RangeSize>,
               ContainerType, std::plus, KeyType, RegType>;

template <typename KeyType, typename RegType, std::size_t RangeSize>
using MultiLocalFWHT =
    MultiLocal<krowkee::transform::FWHTFunctor<RegType, RangeSize>,
               krowkee::sketch::Dense, std::plus, KeyType, RegType>;

}  // namespace stream
}  // namespace krowkee
