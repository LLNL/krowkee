// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_STREAM_DISTRIBUTED_HPP
#define _KROWKEE_STREAM_DISTRIBUTED_HPP

#include <krowkee/hash/util.hpp>

#include <ygm/detail/ygm_ptr.hpp>

#include <ygm/container/map.hpp>

#include <sstream>

namespace krowkee {
namespace stream {

/**
 * Distributedple Sketch
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
    typename... Args>
class Distributed {
 public:
  typedef SketchFunc<RegType, Args...> sf_t;
  typedef ygm::ygm_ptr<sf_t>           sf_ptr_t;
  typedef SketchType<SketchFunc, ContainerType, MergeOp, RegType, ygm::ygm_ptr,
                     Args...>
                                       sk_t;
  typedef DataType<sk_t, ygm::ygm_ptr> data_t;
  typedef Distributed<DataType, SketchType, SketchFunc, ContainerType, MergeOp,
                      KeyType, RegType, Args...>
                              dsk_t;
  typedef ygm::ygm_ptr<dsk_t> dsk_ptr_t;

  typedef ygm::container::map<KeyType, data_t> sk_map_t;
  typedef std::pair<KeyType, data_t>           sk_pair_t;

 private:
  std::size_t _compaction_threshold;
  std::size_t _promotion_threshold;
  sf_ptr_t    _sf_ptr;  /// pointer to the shared sketch functor
  sk_map_t    _sk_map;  /// map of indices to data
  dsk_ptr_t   _pthis;

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
  Distributed(ygm::comm &comm, const sf_ptr_t &sf_ptr,
              const std::size_t compaction_threshold = 128,
              const std::size_t promotion_threshold  = 4096)
      : _compaction_threshold(compaction_threshold),
        _promotion_threshold(promotion_threshold),
        _sk_map(comm,
                data_t{sf_ptr, compaction_threshold, promotion_threshold}),
        _sf_ptr(sf_ptr),
        _pthis(this) {}

  /**
   * Copy constructor.
   */
  Distributed(const dsk_t &rhs)
      : _compaction_threshold(rhs._compaction_threshold),
        _promotion_threshold(rhs._promotion_threshold),
        _sf_ptr(rhs._sf_ptr),
        _sk_map(rhs._sk_map),
        _pthis(this) {}

  static inline std::string name() {
    std::stringstream ss;
    ss << "Distributed " << sk_t::name();
    return ss.str();
  }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << "Distributed " << sk_t::full_name();
    return ss.str();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Sketch Insertion
  //////////////////////////////////////////////////////////////////////////////

  inline void async_insert(const KeyType &key, const data_t &data) {
    _sk_map.async_insert(key, data);
  }

  template <typename... ItemArgs>
  inline void async_update(const KeyType &key, const ItemArgs &...args) {
    auto update_visitor = [](const KeyType &key, data_t &data,
                             const ItemArgs &...args) {
      // std::cout << "starting d1 value: " << kv_pair.second.sk << std::endl;
      data.update(args...);
      // std::cout << "updated d1 value: " << kv_pair.second.sk << std::endl;
    };
    _sk_map.async_visit(key, update_visitor, args...);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Directed Merges
  //////////////////////////////////////////////////////////////////////////////

  inline void async_merge(const KeyType &fwd_key, const KeyType &rec_key) {
    auto forward_visitor = [](auto pmap, const KeyType &forward_key,
                              data_t &forward_data, KeyType receiver_key) {
      auto receive_visitor = [](const KeyType &receiver_key,
                                data_t &receiver_data, data_t forward_data) {
        receiver_data += forward_data;
      };
      pmap->async_visit(receiver_key, receive_visitor, forward_data);
    };
    _sk_map.async_visit(fwd_key, forward_visitor, rec_key);
  }

  inline void async_merge(dsk_t &rhs, const KeyType &rhs_key,
                          const KeyType &lhs_key) {
    auto rhs_visitor = [](const KeyType &rhs_key, data_t &rhs_data,
                          dsk_ptr_t lhs_ptr, KeyType lhs_key) {
      auto lhs_visitor = [](const KeyType &lhs_key, data_t &lhs_data,
                            data_t rhs_data) { lhs_data += rhs_data; };
      lhs_ptr->ygm_map().async_visit(lhs_key, lhs_visitor, rhs_data);
    };
    rhs._sk_map.async_visit(rhs_key, rhs_visitor, _pthis, lhs_key);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  void compactify(const KeyType &key) {
    auto compaction_visitor = [](const KeyType &key, data_t &data) {
      data.compactify();
    };
    _sk_map.async_visit(key, compaction_visitor);
  }

  void compactify() {
    auto compaction_visitor = [](const KeyType &key, data_t &data) {
      data.compactify();
    };
    _sk_map.for_all(compaction_visitor);
  }

  //////////////////////////////////////////////////////////////////////////////
  // For All
  //////////////////////////////////////////////////////////////////////////////

  template <typename Func>
  void for_all(Func func) {
    _sk_map.for_all(func);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  std::size_t local_size() { return _sk_map.local_size(); }
  std::size_t size() { return _sk_map.size(); }

  sk_map_t &ygm_map() { return _sk_map; }

  //////////////////////////////////////////////////////////////////////////////
  // Equality check
  //////////////////////////////////////////////////////////////////////////////

  constexpr bool params_agree(const dsk_t &rhs) const {
    return *_sf_ptr == *rhs._sf_ptr &&
           _promotion_threshold == rhs._promotion_threshold &&
           _compaction_threshold == rhs._compaction_threshold;
  }
};

}  // namespace stream
}  // namespace krowkee

#endif