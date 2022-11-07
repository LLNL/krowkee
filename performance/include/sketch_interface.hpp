// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <graph.hpp>

#include <krowkee/hash/hash.hpp>
#include <krowkee/sketch/interface.hpp>

template <typename SketchType, typename ValueType>
struct hist_sketch_t {
  using sk_t     = typename SketchType::sk_t;
  using sf_t     = typename SketchType::sf_t;
  using sf_ptr_t = typename SketchType::sf_ptr_t;
  hist_sketch_t(const sf_ptr_t     &sf_ptr,
                const std::uint64_t compaction_threshold,
                const std::uint64_t promotion_threshold,
                const parameters_t &params)
      : _sk(sf_ptr, compaction_threshold, promotion_threshold) {}

  void insert(const ValueType &idx) { _sk.insert(idx); }

  std::string static full_name() { return sk_t::full_name(); }

 private:
  SketchType _sk;
};

template <typename SketchType, typename ValueType>
struct graph_sketch_t {
  using sk_t     = typename SketchType::sk_t;
  using sf_t     = typename SketchType::sf_t;
  using sf_ptr_t = typename SketchType::sf_ptr_t;
  graph_sketch_t(const sf_ptr_t     &sf_ptr,
                 const std::uint64_t compaction_threshold,
                 const std::uint64_t promotion_threshold,
                 const parameters_t &params)
      : _sk() {
    for (int i(0); i < params.domain_size; ++i) {
      _sk.push_back(std::make_unique<SketchType>(sf_ptr, compaction_threshold,
                                                 promotion_threshold));
      std::cout << "gets here: " << i << std::endl;
    }
  }

  void insert(const edge_type<ValueType> &edge) {
    std::cout << *_sk[edge.first] << std::endl;
    std::cout << "gets here: " << edge.first << std::endl;
    _sk[edge.first]->insert(edge.second);
  }

  std::string static full_name() { return sk_t::full_name(); }

 private:
  std::vector<std::unique_ptr<SketchType>> _sk;
};

template <typename ValueType>
using dense32_hist_cs_t = hist_sketch_t<
    krowkee::sketch::LocalCountSketch<krowkee::sketch::Dense, std::int32_t>,
    ValueType>;
template <typename ValueType>
using map_sparse32_hist_cs_t =
    hist_sketch_t<krowkee::sketch::LocalCountSketch<
                      krowkee::sketch::MapSparse32, std::int32_t>,
                  ValueType>;
template <typename ValueType>
using map_promotable32_hist_cs_t =
    hist_sketch_t<krowkee::sketch::LocalCountSketch<
                      krowkee::sketch::MapPromotable32, std::int32_t>,
                  ValueType>;
#if __has_include(<boost/container/flat_map.hpp>)
template <typename ValueType>
using flatmap_sparse32_hist_cs_t =
    hist_sketch_t<krowkee::sketch::LocalCountSketch<
                      krowkee::sketch::FlatMapSparse32, std::int32_t>,
                  ValueType>;

template <typename ValueType>
using flatmap_promotable32_hist_cs_t =
    hist_sketch_t<krowkee::sketch::LocalCountSketch<
                      krowkee::sketch::FlatMapPromotable32, std::int32_t>,
                  ValueType>;
#endif

template <typename ValueType>
using dense32_graph_cs_t = graph_sketch_t<
    krowkee::sketch::LocalCountSketch<krowkee::sketch::Dense, std::int32_t>,
    ValueType>;
template <typename ValueType>
using map_sparse32_graph_cs_t =
    graph_sketch_t<krowkee::sketch::LocalCountSketch<
                       krowkee::sketch::MapSparse32, std::int32_t>,
                   ValueType>;
template <typename ValueType>
using map_promotable32_graph_cs_t =
    graph_sketch_t<krowkee::sketch::LocalCountSketch<
                       krowkee::sketch::MapPromotable32, std::int32_t>,
                   ValueType>;
#if __has_include(<boost/container/flat_map.hpp>)
template <typename ValueType>
using flatmap_sparse32_graph_cs_t =
    graph_sketch_t<krowkee::sketch::LocalCountSketch<
                       krowkee::sketch::FlatMapSparse32, std::int32_t>,
                   ValueType>;

template <typename ValueType>
using flatmap_promotable32_graph_cs_t =
    graph_sketch_t<krowkee::sketch::LocalCountSketch<
                       krowkee::sketch::FlatMapPromotable32, std::int32_t>,
                   ValueType>;
#endif
