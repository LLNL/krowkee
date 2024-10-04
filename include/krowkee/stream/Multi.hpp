// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_STREAM_MULTI_HPP
#define _KROWKEE_STREAM_MULTI_HPP

#include <krowkee/hash/util.hpp>

#include <map>
#include <sstream>

namespace krowkee {
namespace stream {

/**
 * Multiple Sketch
 *
 * Holds the core-local rows of a sketch along with an associated transform.
 */
template <
    template <typename, template <typename> class> class DataType,
    template <template <typename, typename...> class,
              template <typename, typename> class, template <typename> class,
              typename, template <typename> class, typename...>
    class SketchType,
    template <typename, typename...> class SketchFunc,
    template <typename, typename> class ContainerType,
    template <typename> class MergeOp, typename KeyType, typename RegType,
    template <typename> class PtrType, typename... Args>
class Multi {
 public:
  using register_type      = RegType;
  using transform_type     = SketchFunc<register_type, Args...>;
  using transform_ptr_type = PtrType<transform_type>;
  using sketch_type        = SketchType<SketchFunc, ContainerType, MergeOp,
                                 register_type, PtrType, Args...>;
  using data_type          = DataType<sketch_type, PtrType>;
  using self_type = Multi<DataType, SketchType, SketchFunc, ContainerType,
                          MergeOp, KeyType, register_type, PtrType, Args...>;

  using map_type  = std::map<KeyType, data_type>;
  using pair_type = std::pair<KeyType, data_type>;

 private:
  transform_ptr_type _transform_ptr;  /// pointer to the shared sketch functor
  map_type           _sketch_map;     /// map of indices to data
  std::size_t        _compaction_threshold;
  std::size_t        _promotion_threshold;

 public:
  /**
   * Initialize hash functors and register vector.
   *
   * @param sf_ptr the sketch functor.
   * @param compaction_threshold the size at which compacting maps compact. Only
   *        used by Sparse and Promotable (in sparse mode) sketches.
   * @param promotion_threshold the size at which to promote a sparse sketch to
   *        a dense sketch. Only used by Promotable sketches.
   */
  Multi(const transform_ptr_type &sf_ptr,
        const std::size_t         compaction_threshold = 128,
        const std::size_t         promotion_threshold  = 4096)
      : _transform_ptr(sf_ptr),
        _compaction_threshold(compaction_threshold),
        _promotion_threshold(promotion_threshold) {}

  /**
   * Copy constructor.
   */
  Multi(const self_type &rhs)
      : _transform_ptr(rhs._transform_ptr),
        _sketch_map(rhs._sketch_map),
        _compaction_threshold(rhs._compaction_threshold),
        _promotion_threshold(rhs._promotion_threshold) {}

  static inline std::string name() {
    std::stringstream ss;
    ss << "Multi " << sketch_type::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Multi " << sketch_type::full_name();
    return ss.str();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Sketch Insertion
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Insert item into the specified sketch registers.
   *
   * Inserts `x` into the sketch specified by `id`. Creates a new sketch for
   * `id` if one does not already exist.
   *
   * @tparam ItemArgs... types of parameters of the stream object to be
   *     inserted. Will be used to construct a krowkee::stream::Element object.
   *
   * @param key the row identifier.
   * @param x the object to be inserted.
   * @param multiplicity the number of insert repetitions to perform. Can be
   *     negative. Default is `1`.
   */
  template <typename... ItemArgs>
  inline void insert(const KeyType &key, const ItemArgs &...args) {
    auto itr(_sketch_map.find(key));
    if (itr != std::end(_sketch_map)) {
      itr->second.update(args...);
    } else {
      data_type data{_transform_ptr, _compaction_threshold,
                     _promotion_threshold, args...};
      _sketch_map[key] = data;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  void compactify(const KeyType key) {
    auto itr(_sketch_map.find(key));
    if (itr != std::end(_sketch_map)) {
      itr->second.compactify();
    } else {
      std::stringstream ss;
      ss << "error: key " << key << " does not exist!";
      throw std::invalid_argument(ss.str());
    }
  }

  void compactify() {
    std::for_each(std::begin(_sketch_map), std::end(_sketch_map),
                  [](auto &p) { p.second.compactify(); });
  }

  //////////////////////////////////////////////////////////////////////////////
  // Sketch Access
  //////////////////////////////////////////////////////////////////////////////

  data_type &operator[](const KeyType key) {
    auto itr(_sketch_map.find(key));
    if (itr != std::end(_sketch_map)) {
      return itr->second;
    } else {
      data_type data{_transform_ptr, _compaction_threshold,
                     _promotion_threshold};
      _sketch_map[key] = data;
      return _sketch_map[key];
    }
  }

  data_type &at(const KeyType key) {
    auto itr(_sketch_map.find(key));
    if (itr != std::end(_sketch_map)) {
      return itr->second;
    } else {
      std::stringstream ss;
      ss << "error: key " << key << " does not exist!";
      throw std::invalid_argument(ss.str());
    }
  }

  constexpr const data_type &at(const KeyType key) const {
    auto itr(_sketch_map.find(key));
    if (itr != std::end(_sketch_map)) {
      return itr->second.sk;
    } else {
      std::stringstream ss;
      ss << "error: key " << key << " does not exist!";
      throw std::invalid_argument(ss.str());
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  constexpr typename map_type::iterator begin() {
    return std::begin(_sketch_map);
  }
  constexpr typename map_type::const_iterator begin() const {
    return std::cbegin(_sketch_map);
  }
  constexpr typename map_type::const_iterator cbegin() const {
    return std::cbegin(_sketch_map);
  }
  constexpr typename map_type::iterator end() { return std::end(_sketch_map); }
  constexpr typename map_type::const_iterator end() const {
    return std::cend(_sketch_map);
  }
  constexpr typename map_type::const_iterator cend() {
    return std::cend(_sketch_map);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  constexpr std::size_t size() const { return _sketch_map.size(); }

  //////////////////////////////////////////////////////////////////////////////
  // Equality check
  //////////////////////////////////////////////////////////////////////////////

  constexpr bool _params_agree(const self_type &other) const {
    return _transform_ptr == other._transform_ptr &&
           _promotion_threshold == other._promotion_threshold &&
           _compaction_threshold == other._compaction_threshold;
  }

  constexpr bool _data_agree(const self_type &other) const {
    for (auto pair : _sketch_map) {
      auto other_itr = other._sketch_map.find(pair.first);
      if (other_itr == std::end(other._sketch_map) ||
          pair.second != other_itr->second) {
        return false;
      }
    }
    return true;
  }

  friend constexpr bool operator==(const self_type &lhs, const self_type &rhs) {
    return lhs._params_agree(rhs) && lhs._data_agree(rhs) &&
           lhs.size() == rhs.size();
  }

  friend constexpr bool operator!=(const self_type &lhs, const self_type &rhs) {
    return !(lhs == rhs);
  }
};

}  // namespace stream
}  // namespace krowkee

#endif