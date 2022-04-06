// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other
// krowkee Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#ifndef _KROWKEE_SKETCH_PROMOTABLE_HPP
#define _KROWKEE_SKETCH_PROMOTABLE_HPP

#include <krowkee/sketch/Dense.hpp>
#include <krowkee/sketch/Sparse.hpp>

#if __has_include(<cereal/types/memory.hpp>)
#include <cereal/types/memory.hpp>
#endif

#include <memory>

namespace krowkee {
namespace sketch {

enum class promotable_mode_t : std::uint8_t { sparse, dense };

/**
 * Sketch Dense/Sparse union data structure
 *
 * Provides a unified interface to the Sparse and Dense containers, allowing for
 * seamless "promotion" of a Sparse representation to a Dense one.
 */
template <typename RegType, typename MergeOp,
          template <typename, typename> class MapType, typename KeyType>
class Promotable {
 public:
  typedef krowkee::sketch::Dense<RegType, MergeOp> dense_t;
  typedef std::unique_ptr<dense_t>                 dense_ptr_t;
  typedef krowkee::sketch::Sparse<RegType, MergeOp, MapType, KeyType> sparse_t;
  typedef typename sparse_t::map_t                                    map_t;
  typedef std::unique_ptr<sparse_t>                      sparse_ptr_t;
  typedef Promotable<RegType, MergeOp, MapType, KeyType> promotable_t;

 private:
  dense_ptr_t       _dense_ptr;
  sparse_ptr_t      _sparse_ptr;
  std::size_t       _promotion_threshold;
  promotable_mode_t _mode;

 public:
  /**
   * Initialize the size parameters and create the base container.
   *
   * @param range_size size argument for constructing dense container.
   * @param compaction_threshold size argument for constructing sparse
   *     container.
   * @param mode indicates whether to create a sparse or a dense container.
   */
  Promotable(const std::size_t range_size,
             const std::size_t compaction_threshold,
             const std::size_t promotion_threshold)
      : _promotion_threshold(promotion_threshold),
        _mode(promotable_mode_t::sparse),
        _sparse_ptr(
            std::make_unique<sparse_t>(range_size, compaction_threshold)) {}

  /**
   * Copy constructor.
   *
   * Must explicitly copy contents of rhs's container, not just copy the
   * pointer.
   *
   * @param rhs container to be copied.
   */
  Promotable(const promotable_t &rhs)
      : _promotion_threshold(rhs._promotion_threshold), _mode(rhs._mode) {
    if (_mode == promotable_mode_t::sparse) {
      sparse_ptr_t sparse_ptr(std::make_unique<sparse_t>(*rhs._sparse_ptr));
      std::swap(sparse_ptr, _sparse_ptr);
    } else {
      dense_ptr_t dense_ptr(std::make_unique<dense_t>(*rhs._dense_ptr));
      std::swap(dense_ptr, _dense_ptr);
    }
  }

  /**
   * default constructor (only use for move constructor!)
   */
  Promotable()
      : _mode(promotable_mode_t::sparse), _sparse_ptr(), _dense_ptr() {}

  // /**
  //  * move constructor
  //  *
  //  * @param rhs r-value promotable_t to be destructively copied.
  //  */
  // Promotable(promotable_t &&rhs) noexcept : Promotable() {
  //   std::swap(*this, rhs);
  // }

  //////////////////////////////////////////////////////////////////////////////
  // Swaps
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Swap boilerplate.
   */
  friend void swap(promotable_t &lhs, promotable_t &rhs) {
    std::swap(lhs._promotion_threshold, rhs._promotion_threshold);
    std::swap(lhs._mode, rhs._mode);
    if (lhs._mode == promotable_mode_t::sparse) {
      std::swap(lhs._sparse_ptr, rhs._sparse_ptr);
    } else {
      std::swap(lhs._dense_ptr, rhs._dense_ptr);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cereal Archives
  //////////////////////////////////////////////////////////////////////////////

#if __has_include(<cereal/types/memory.hpp>)
  template <class Archive>
  void save(Archive &oarchive) const {
    oarchive(_promotion_threshold, _mode);
    if (_mode == promotable_mode_t::sparse) {
      oarchive(_sparse_ptr);
    } else {
      oarchive(_dense_ptr);
    }
  }

  template <class Archive>
  void load(Archive &iarchive) {
    iarchive(_promotion_threshold, _mode);
    if (_mode == promotable_mode_t::sparse) {
      iarchive(_sparse_ptr);
    } else {
      iarchive(_dense_ptr);
    }
  }
#endif

  //////////////////////////////////////////////////////////////////////////////
  // Assignment
  //////////////////////////////////////////////////////////////////////////////

  /**
   * copy-and-swap assignment operator
   *
   * https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
   */
  promotable_t &operator=(promotable_t rhs) {
    std::swap(*this, rhs);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Getters
  //////////////////////////////////////////////////////////////////////////////

  static inline std::string name() { return "Promotable"; }

  static inline std::string full_name() {
    std::stringstream ss;
    ss << name() << " using " << krowkee::hash::type_name<map_t>();
    return ss.str();
  }

  constexpr std::size_t size() const {
    return (_mode == promotable_mode_t::sparse) ? _sparse_ptr->size()
                                                : _dense_ptr->size();
  }

  constexpr bool is_compact() const {
    return (_mode == promotable_mode_t::sparse) ? _sparse_ptr->is_compact()
                                                : true;
  }

  constexpr bool is_sparse() const {
    return (_mode == promotable_mode_t::sparse) ? true : false;
  }

  constexpr std::size_t reg_size() const { return sizeof(RegType); }

  constexpr std::size_t get_promotion_threshold() const {
    return _promotion_threshold;
  }

  constexpr promotable_mode_t get_mode() const { return _mode; }

  constexpr std::size_t get_compaction_threshold() const {
    return (_mode == promotable_mode_t::sparse)
               ? _sparse_ptr->get_compaction_threshold()
               : _dense_ptr->get_compaction_threshold();
  }

  std::vector<RegType> get_register_vector() const {
    return (_mode == promotable_mode_t::sparse)
               ? _sparse_ptr->get_register_vector()
               : _dense_ptr->get_register_vector();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Equality operators
  //////////////////////////////////////////////////////////////////////////////

  constexpr bool same_parameters(const promotable_t &rhs) const {
    return _promotion_threshold == rhs._promotion_threshold;
  }

  /**
   * @note[bwp] do we want to be able to compare sparse to dense containers
   * directly? no for now but will want to revisit.
   */
  friend constexpr bool operator==(const promotable_t &lhs,
                                   const promotable_t &rhs) {
    if (lhs.same_parameters(rhs) == false || lhs._mode != rhs._mode) {
      return false;
    }
    if (lhs._mode == promotable_mode_t::sparse) {
      return *lhs._sparse_ptr == *rhs._sparse_ptr;
    } else {
      return *lhs._dense_ptr == *rhs._dense_ptr;
    }
  }
  friend constexpr bool operator!=(const promotable_t &lhs,
                                   const promotable_t &rhs) {
    return !operator==(lhs, rhs);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Compaction
  //////////////////////////////////////////////////////////////////////////////

  void compactify() {
    if (_mode == promotable_mode_t::sparse) {
      _sparse_ptr->compactify();
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Erase
  //////////////////////////////////////////////////////////////////////////////

  inline void erase(const std::uint64_t index) {
    if (_mode == promotable_mode_t::sparse) {
      _sparse_ptr->erase(index);
    } else {
      _dense_ptr->erase(index);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register iterators
  //////////////////////////////////////////////////////////////////////////////

  constexpr auto begin() {
    if (_mode == promotable_mode_t::sparse) {
      return std::begin(*_sparse_ptr);
    } else {
      return std::begin(*_dense_ptr);
    }
  }
  constexpr auto begin() const {
    if (_mode == promotable_mode_t::sparse) {
      return std::cbegin(*_sparse_ptr);
    } else {
      return std::cbegin(*_dense_ptr);
    }
  }
  constexpr auto cbegin() const {
    if (_mode == promotable_mode_t::sparse) {
      return std::cbegin(*_sparse_ptr);
    } else {
      return std::cbegin(*_dense_ptr);
    }
  }
  constexpr auto end() {
    if (_mode == promotable_mode_t::sparse) {
      return std::end(*_sparse_ptr);
    } else {
      return std::end(*_dense_ptr);
    }
  }
  constexpr auto end() const {
    if (_mode == promotable_mode_t::sparse) {
      return std::cend(*_sparse_ptr);
    } else {
      return std::cend(*_dense_ptr);
    }
  }
  constexpr auto cend() const {
    if (_mode == promotable_mode_t::sparse) {
      return std::cend(*_sparse_ptr);
    } else {
      return std::cend(*_dense_ptr);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accessors
  //////////////////////////////////////////////////////////////////////////////

  RegType &operator[](const std::uint64_t index) {
    if (_mode == promotable_mode_t::sparse) {
      if (size() == _promotion_threshold) {
        compactify();
        promote();
        return (*_dense_ptr)[index];
      }
      return (*_sparse_ptr)[index];
    } else {
      return (*_dense_ptr)[index];
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Merge Operators
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Merge other promotable_t into `this`.
   *
   * @param rhs the other promotable_t. Must
   *
   * @throws std::invalid_argument if the parameters do not agree.
   */
  promotable_t &operator+=(const promotable_t &rhs) {
    if (same_parameters(rhs) == false) {
      throw std::invalid_argument(
          "containers do not have congruent parameters!");
    }
    if (_mode == rhs._mode) {
      if (_mode == promotable_mode_t::sparse) {
        // sparse_t tmp(*rhs._sparse_ptr);
        // *_sparse_ptr += tmp;
        *_sparse_ptr += *rhs._sparse_ptr;
        if (size() >= _promotion_threshold) {
          promote();
        }
      } else {
        *_dense_ptr += *rhs._dense_ptr;
      }
    } else {
      if (_mode == promotable_mode_t::sparse) {
        // we are sparse; promote to dense and add rhs._dense_ptr
        // This will be slow; try to avoid it.
        promote();
        *_dense_ptr += *rhs._dense_ptr;
      } else {
        // we are dense; add rhs._sparse_ptr
        merge_from_sparse(rhs);
      }
    }
    return *this;
  }

  inline friend promotable_t operator+(const promotable_t &lhs,
                                       const promotable_t &rhs) {
    if (rhs._mode == promotable_mode_t::dense) {
      promotable_t ret(rhs);
      ret += lhs;
      return ret;
    }
    promotable_t ret(lhs);
    ret += rhs;
    return ret;
  }

  /**
   * Promote sparse container into a dense container.
   *
   * Initializes dense pointer while releasing sparse pointer.
   *
   * @throws std::logic_error if the container is not in sparse mode.
   * @throws std::logic_error if the sparse container is not compacted.
   */
  void promote() {
    if (_mode != promotable_mode_t::sparse) {
      throw std::logic_error("Attempt to promote non-sparse container!");
    }
    if (_sparse_ptr->is_compact() == false) {
      throw std::logic_error("Attempt to promote uncompacted container!");
    }

    _dense_ptr = std::make_unique<dense_t>(_sparse_ptr->range_size());
    merge_from_sparse(*this);
    _sparse_ptr.reset();
    _mode = promotable_mode_t::dense;
  }

  /**
   * Incorporate the information in a sparse container into a dense container.
   *
   * Initializes dense pointer while releasing sparse pointer.
   *
   * @throws std::logic_error if the rhs container is not in sparse mode.
   */
  void merge_from_sparse(const promotable_t &rhs) {
    if (rhs._mode != promotable_mode_t::sparse) {
      throw std::logic_error("Attempt to dense merge a non-sparse rhs!");
    }
    if (rhs.is_compact() == false) {
      throw std::logic_error("Attempt to dense merge a non-compact rhs!");
    }
    for_each(*rhs._sparse_ptr, [&](const auto &p) {
      RegType &val((*_dense_ptr)[p.first]);
      val = MergeOp()(val, p.second);
    });
  }

  //////////////////////////////////////////////////////////////////////////////
  // I/O Operators
  //////////////////////////////////////////////////////////////////////////////
  friend std::ostream &operator<<(std::ostream &os, const promotable_t &con) {
    if (con._mode == promotable_mode_t::sparse) {
      os << *con._sparse_ptr;
    } else {
      os << *con._dense_ptr;
    }
    return os;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Accumulation
  //////////////////////////////////////////////////////////////////////////////

  template <typename RetType>
  friend RetType accumulate(const promotable_t &con, const RetType init) {
    if (con._mode == promotable_mode_t::sparse) {
      return accumulate(*con._sparse_ptr, init);
    } else {
      return accumulate(*con._dense_ptr, init);
    }
  }
};

}  // namespace sketch
}  // namespace krowkee

#endif