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

template <template <typename, typename...> class SketchFunc,
          template <typename, typename> class ContainerType,
          template <typename> class MergeOp, typename KeyType, typename RegType,
          typename... Args>
using MultiLocal =
    Multi<CountingSummary, krowkee::sketch::Sketch, SketchFunc, ContainerType,
          MergeOp, KeyType, RegType, std::shared_ptr, Args...>;

template <template <typename, typename> class ContainerType, typename KeyType,
          typename RegType>
using MultiLocalCountSketch =
    MultiLocal<krowkee::transform::CountSketchFunctor, ContainerType, std::plus,
               KeyType, RegType,
               krowkee::hash::CountSketchHash<krowkee::hash::MulAddShift>>;

template <typename KeyType, typename RegType>
using MultiLocalFWHT =
    MultiLocal<krowkee::transform::FWHTFunctor, krowkee::sketch::Dense,
               std::plus, KeyType, RegType>;

}  // namespace stream
}  // namespace krowkee
