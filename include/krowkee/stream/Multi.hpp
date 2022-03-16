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
  typedef SketchFunc<RegType, Args...> sf_t;
  typedef PtrType<sf_t>                sf_ptr_t;
  typedef SketchType<SketchFunc, ContainerType, MergeOp, RegType, PtrType,
                     Args...>
                                  sk_t;
  typedef DataType<sk_t, PtrType> data_t;
  typedef Multi<DataType, SketchType, SketchFunc, ContainerType, MergeOp,
                KeyType, RegType, PtrType, Args...>
      msk_t;

  typedef std::map<KeyType, data_t>  sk_map_t;
  typedef std::pair<KeyType, data_t> sk_pair_t;

 private:
  sf_ptr_t    _sf_ptr;  /// pointer to the shared sketch functor
  sk_map_t    _sk_map;  /// map of indices to data
  std::size_t _compaction_threshold;
  std::size_t _promotion_threshold;

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
  Multi(const sf_ptr_t &sf_ptr, const std::size_t compaction_threshold = 128,
        const std::size_t promotion_threshold = 4096)
      : _sf_ptr(sf_ptr),
        _compaction_threshold(compaction_threshold),
        _promotion_threshold(promotion_threshold) {}

  /**
   * Copy constructor.
   */
  Multi(const msk_t &rhs)
      : _sf_ptr(rhs._sf_ptr),
        _sk_map(rhs._sk_map),
        _compaction_threshold(rhs._compaction_threshold),
        _promotion_threshold(rhs._promotion_threshold) {}

  static inline std::string name() {
    std::stringstream ss;
    ss << "Multi " << sk_t::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Multi " << sk_t::full_name();
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
    auto itr(_sk_map.find(key));
    if (itr != std::end(_sk_map)) {
      itr->second.update(args...);
    } else {
      data_t data{_sf_ptr, _compaction_threshold, _promotion_threshold,
                  args...};
      _sk_map[key] = data;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  void compactify(const KeyType key) {
    auto itr(_sk_map.find(key));
    if (itr != std::end(_sk_map)) {
      itr->second.compactify();
    } else {
      std::stringstream ss;
      ss << "error: key " << key << " does not exist!";
      throw std::invalid_argument(ss.str());
    }
  }

  void compactify() {
    std::for_each(std::begin(_sk_map), std::end(_sk_map),
                  [](auto &p) { p.second.compactify(); });
  }

  //////////////////////////////////////////////////////////////////////////////
  // Sketch Access
  //////////////////////////////////////////////////////////////////////////////

  data_t &operator[](const KeyType key) {
    auto itr(_sk_map.find(key));
    if (itr != std::end(_sk_map)) {
      return itr->second;
    } else {
      data_t data{_sf_ptr, _compaction_threshold, _promotion_threshold};
      _sk_map[key] = data;
      return _sk_map[key];
    }
  }

  data_t &at(const KeyType key) {
    auto itr(_sk_map.find(key));
    if (itr != std::end(_sk_map)) {
      return itr->second;
    } else {
      std::stringstream ss;
      ss << "error: key " << key << " does not exist!";
      throw std::invalid_argument(ss.str());
    }
  }

  constexpr const data_t &at(const KeyType key) const {
    auto itr(_sk_map.find(key));
    if (itr != std::end(_sk_map)) {
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

  constexpr typename sk_map_t::iterator begin() { return std::begin(_sk_map); }
  constexpr typename sk_map_t::const_iterator begin() const {
    return std::cbegin(_sk_map);
  }
  constexpr typename sk_map_t::const_iterator cbegin() const {
    return std::cbegin(_sk_map);
  }
  constexpr typename sk_map_t::iterator end() { return std::end(_sk_map); }
  constexpr typename sk_map_t::const_iterator end() const {
    return std::cend(_sk_map);
  }
  constexpr typename sk_map_t::const_iterator cend() {
    return std::cend(_sk_map);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  constexpr std::size_t size() const { return _sk_map.size(); }

  //////////////////////////////////////////////////////////////////////////////
  // Equality check
  //////////////////////////////////////////////////////////////////////////////

  constexpr bool _params_agree(const msk_t &other) const {
    return _sf_ptr == other._sf_ptr &&
           _promotion_threshold == other._promotion_threshold &&
           _compaction_threshold == other._compaction_threshold;
  }

  constexpr bool _data_agree(const msk_t &other) const {
    for (auto pair : _sk_map) {
      auto other_itr = other._sk_map.find(pair.first);
      if (other_itr == std::end(other._sk_map) ||
          pair.second != other_itr->second) {
        return false;
      }
    }
    return true;
  }

  friend constexpr bool operator==(const msk_t &lhs, const msk_t &rhs) {
    return lhs._params_agree(rhs) && lhs._data_agree(rhs) &&
           lhs.size() == rhs.size();
  }

  friend constexpr bool operator!=(const msk_t &lhs, const msk_t &rhs) {
    return !(lhs == rhs);
  }
};

}  // namespace stream
}  // namespace krowkee

#endif