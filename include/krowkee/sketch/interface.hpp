// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <krowkee/hash/countsketch.hpp>
#include <krowkee/hash/hash.hpp>

#include <krowkee/transform/CountSketch.hpp>
#include <krowkee/transform/FWHT.hpp>

#include <krowkee/sketch/Dense.hpp>
#include <krowkee/sketch/Promotable.hpp>
#include <krowkee/sketch/Sparse.hpp>

#include <krowkee/sketch/Sketch.hpp>

#if __has_include(<boost/container/flat_map.hpp>)
#include <boost/container/flat_map.hpp>
#endif

#include <map>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
// Sketch Type Presets
////////////////////////////////////////////////////////////////////////////////

namespace krowkee {
namespace sketch {

template <typename SketchFunc,
          template <typename, typename> class ContainerType,
          template <typename> class MergeOp, template <typename> class PtrType,
          typename... Args>
using LocalSketch =
    Sketch<SketchFunc, ContainerType, MergeOp, PtrType, Args...>;

template <template <typename, typename> class ContainerType, typename RegType,
          std::size_t RangeSize,
          template <typename> class PtrType = std::shared_ptr>
using CountSketch =
    LocalSketch<krowkee::transform::CountSketchFunctor<
                    RegType, krowkee::hash::CountSketchHash, RangeSize>,
                ContainerType, std::plus, PtrType>;

template <typename RegType, std::size_t RangeSize,
          template <typename> class PtrType = std::shared_ptr>
using FWHT = LocalSketch<krowkee::transform::FWHTFunctor<RegType, RangeSize>,
                         krowkee::sketch::Dense, std::plus, PtrType>;

}  // namespace sketch
}  // namespace krowkee

////////////////////////////////////////////////////////////////////////////////
// Sparse Container Presets
////////////////////////////////////////////////////////////////////////////////

namespace krowkee {
namespace sketch {

template <typename RegType, typename MergeOp, typename KeyType>
using MapSparse = Sparse<RegType, MergeOp, std::map, KeyType>;

template <typename RegType, typename MergeOp>
using MapSparse32 = MapSparse<RegType, MergeOp, std::uint32_t>;

#if __has_include(<boost/container/flat_map.hpp>)
template <typename RegType, typename MergeOp, typename KeyType>
using FlatMapSparse =
    Sparse<RegType, MergeOp, boost::container::flat_map, KeyType>;

template <typename RegType, typename MergeOp>
using FlatMapSparse32 = FlatMapSparse<RegType, MergeOp, std::uint32_t>;
#endif

}  // namespace sketch
}  // namespace krowkee

////////////////////////////////////////////////////////////////////////////////
// Promotable Container Presets
////////////////////////////////////////////////////////////////////////////////

namespace krowkee {
namespace sketch {

template <typename RegType, typename MergeOp, typename KeyType>
using MapPromotable = Promotable<RegType, MergeOp, std::map, KeyType>;

template <typename RegType, typename MergeOp>
using MapPromotable32 = MapPromotable<RegType, MergeOp, std::uint32_t>;

#if __has_include(<boost/container/flat_map.hpp>)
template <typename RegType, typename MergeOp, typename KeyType>
using FlatMapPromotable =
    Promotable<RegType, MergeOp, boost::container::flat_map, KeyType>;

template <typename RegType, typename MergeOp>
using FlatMapPromotable32 = FlatMapPromotable<RegType, MergeOp, std::uint32_t>;
#endif

}  // namespace sketch
}  // namespace krowkee
