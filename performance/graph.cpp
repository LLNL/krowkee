// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <parameters.hpp>
#include <sampling.hpp>
#include <sketch_interface.hpp>
#include <timing.hpp>
#include <types.hpp>
#include <utils.hpp>

template <typename ValueType>
struct vector_vector_graph_t : public vector_t<std::vector<ValueType>> {
  vector_vector_graph_t(const parameters_t &params)
      : vector_t<std::vector<ValueType>>(
            params.domain_size, std::vector<ValueType>(params.domain_size)) {}

  void insert(const edge_type<ValueType> edge) {
    this->check_bounds(edge.first);
    this->check_bounds(edge.second);
    this->_vec[edge.first][edge.second]++;
  }

  static std::string name() { return "std::vector<std::vector>"; }

  std::vector<ValueType> &operator[](const int i) { return this->_vec[i]; }
};

// key type must equal value type because of how edge_type is constructed
template <typename ValueType>
struct map_vector_graph_t : public map_t<ValueType, std::vector<ValueType>> {
  map_vector_graph_t(const parameters_t &params)
      : map_t<ValueType, std::vector<ValueType>>(params.domain_size),
        _default_value(params.domain_size) {}

  void insert(const edge_type<ValueType> edge) {
    const auto [src, dst] = edge;
    this->check_bounds(src);
    this->check_bounds(dst);
    auto range = this->_map.equal_range(src);
    if (range.first == range.second) {
      this->_map.insert(std::make_pair(src, _default_value));
    }
    this->_map[src][dst]++;
  }

  static std::string name() { return "std::map<std::vector>"; }

 protected:
  std::vector<ValueType> _default_value;
};

// key type must equal value type because of how edge_type is constructed
template <typename ValueType>
struct map_set_graph_t : public map_t<ValueType, std::set<ValueType>> {
  map_set_graph_t(const parameters_t &params)
      : map_t<ValueType, std::set<ValueType>>(params.domain_size) {}

  void insert(const edge_type<ValueType> edge) {
    const auto [src, dst] = edge;
    this->check_bounds(src);
    this->check_bounds(dst);
    this->_map[src].insert(dst);
  }

  static std::string name() { return "std::map<std::set>"; }
};

void benchmark(const parameters_t &params) {
  using vertex_t = std::uint64_t;
  using edge_t   = edge_type<vertex_t>;
  std::vector<edge_t> samples(make_edge_samples<vertex_t>(params));

#if __has_include(<boost/container/flat_map.hpp>)
  auto sk_profiles =
      profile_sketch_histogram<edge_t, dense32_cs_vector_graph_t<vertex_t>,
                               map_sparse32_cs_vector_graph_t<vertex_t>,
                               map_promotable32_cs_vector_graph_t<vertex_t>,
                               flatmap_sparse32_cs_vector_graph_t<vertex_t>,
                               flatmap_promotable32_cs_vector_graph_t<vertex_t>,
                               dense32_cs_map_graph_t<vertex_t>,
                               map_sparse32_cs_map_graph_t<vertex_t>,
                               map_promotable32_cs_map_graph_t<vertex_t>,
                               flatmap_sparse32_cs_map_graph_t<vertex_t>,
                               flatmap_promotable32_cs_map_graph_t<vertex_t>>(
          samples, params);
#else
  auto sk_profiles =
      profile_sketch_histogram<edge_t, dense32_cs_vector_graph_t<vertex_t>,
                               map_sparse32_cs_vector_graph_t<vertex_t>,
                               map_promotable32_cs_graph_t<vertex_t>,
                               dense32_cs_map_graph_t<vertex_t>,
                               map_sparse32_cs_map_graph_t<vertex_t>,
                               map_promotable32_cs_map_graph_t<vertex_t>>(
          samples, params);
#endif

  auto container_profiles =
      profile_container_histogram<edge_t, vector_vector_graph_t<vertex_t>,
                                  map_vector_graph_t<vertex_t>,
                                  map_set_graph_t<vertex_t>>(samples, params);
  print_profiles(sk_profiles, container_profiles);
}

int main(int argc, char **argv) {
  parameters_t params = parse_args(argc, argv);

  std::cout << params << std::endl;

  benchmark(params);
  return 0;
}