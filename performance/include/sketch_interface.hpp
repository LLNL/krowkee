// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <graph.hpp>
#include <parameters.hpp>

#include <krowkee/hash/hash.hpp>
#include <krowkee/sketch/interface.hpp>

template <typename SketchType, typename ValueType>
struct HistSketch {
  using sketch_type        = SketchType;
  using transform_type     = typename sketch_type::transform_type;
  using transform_ptr_type = typename sketch_type::transform_ptr_type;
  HistSketch(const transform_ptr_type &transform_ptr, const Parameters &params)
      : _sketch(transform_ptr, params.compaction_threshold,
                params.promotion_threshold) {}

  void insert(const ValueType &idx) { _sketch.insert(idx); }

  std::string static full_name() { return sketch_type::full_name(); }

 private:
  sketch_type _sketch;
};

template <typename SketchType, typename ValueType>
struct VectorGraphSketch {
  using sketch_type        = SketchType;
  using transform_type     = typename sketch_type::transform_type;
  using transform_ptr_type = typename sketch_type::transform_ptr_type;
  VectorGraphSketch(const transform_ptr_type &transform_ptr,
                    const Parameters         &params)
      : _sketch_vec() {
    for (int i(0); i < params.domain_size; ++i) {
      _sketch_vec.push_back(std::make_unique<sketch_type>(
          transform_ptr, params.compaction_threshold,
          params.promotion_threshold));
    }
  }

  void insert(const edge_type<ValueType> &edge) {
    _sketch_vec[edge.first]->insert(edge.second);
  }

  std::string static full_name() {
    std::stringstream ss;
    ss << "std::vector graph of " << sketch_type::full_name();
    return ss.str();
  }

 private:
  std::vector<std::unique_ptr<SketchType>> _sketch_vec;
};

template <typename SketchType, typename ValueType>
struct MapGraphSketch {
  using sketch_type        = SketchType;
  using transform_type     = typename sketch_type::transform_type;
  using transform_ptr_type = typename sketch_type::transform_ptr_type;

  MapGraphSketch(const transform_ptr_type &transform_ptr,
                 const Parameters         &params)
      : _sketch_map(),
        _transform_ptr(transform_ptr),
        _compaction_threshold(params.compaction_threshold),
        _promotion_threshold(params.promotion_threshold) {}

  void insert(const edge_type<ValueType> &edge) {
    auto [src, dst] = edge;
    auto range      = _sketch_map.equal_range(src);
    if (range.first == range.second) {
      _sketch_map.insert(std::make_pair(
          src,
          std::make_unique<sketch_type>(_transform_ptr, _compaction_threshold,
                                        _promotion_threshold)));
    }
    _sketch_map[src]->insert(dst);
  }

  std::string static full_name() {
    std::stringstream ss;
    ss << "std::map graph of " << sketch_type::full_name();
    return ss.str();
  }

 private:
  std::map<ValueType, std::unique_ptr<sketch_type>> _sketch_map;
  transform_ptr_type                                _transform_ptr;
  std::uint64_t                                     _compaction_threshold;
  std::uint64_t                                     _promotion_threshold;
};

// histogram sketch types

const static std::size_t RANGE_SIZE(32);
const static std::size_t REPLICATION_COUNT(1);

template <typename ValueType>
using dense32_cs_hist =
    HistSketch<krowkee::sketch::SparseJLT<krowkee::sketch::Dense, std::int32_t,
                                          RANGE_SIZE, REPLICATION_COUNT>,
               ValueType>;
template <typename ValueType>
using map_sparse32_cs_hist = HistSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
template <typename ValueType>
using map_promotable32_cs_hist = HistSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapPromotable32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#if __has_include(<boost/container/flat_map.hpp>)
template <typename ValueType>
using flatmap_sparse32_cs_hist = HistSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;

template <typename ValueType>
using flatmap_promotable32_cs_hist = HistSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapPromotable32,
                               std::int32_t, RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#endif

// vector graph sketch types

template <typename ValueType>
using dense32_cs_vector_graph = VectorGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::Dense, std::int32_t, RANGE_SIZE,
                               REPLICATION_COUNT>,
    ValueType>;
template <typename ValueType>
using map_sparse32_cs_vector_graph = VectorGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
template <typename ValueType>
using map_promotable32_cs_vector_graph = VectorGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapPromotable32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#if __has_include(<boost/container/flat_map.hpp>)
template <typename ValueType>
using flatmap_sparse32_cs_vector_graph = VectorGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;

template <typename ValueType>
using flatmap_promotable32_cs_vector_graph = VectorGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapPromotable32,
                               std::int32_t, RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#endif

// map graph sketch types

template <typename ValueType>
using dense32_cs_map_graph = MapGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::Dense, std::int32_t, RANGE_SIZE,
                               REPLICATION_COUNT>,
    ValueType>;
template <typename ValueType>
using map_sparse32_cs_map_graph = MapGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
template <typename ValueType>
using map_promotable32_cs_map_graph = MapGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::MapPromotable32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#if __has_include(<boost/container/flat_map.hpp>)
template <typename ValueType>
using flatmap_sparse32_cs_map_graph = MapGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapSparse32, std::int32_t,
                               RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;

template <typename ValueType>
using flatmap_promotable32_cs_map_graph = MapGraphSketch<
    krowkee::sketch::SparseJLT<krowkee::sketch::FlatMapPromotable32,
                               std::int32_t, RANGE_SIZE, REPLICATION_COUNT>,
    ValueType>;
#endif
