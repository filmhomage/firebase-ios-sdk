/*
 * Copyright 2018 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_TREE_SORTED_MAP_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_TREE_SORTED_MAP_H_

#include <assert.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include "Firestore/core/src/firebase/firestore/immutable/llrb_node.h"
#include "Firestore/core/src/firebase/firestore/immutable/llrb_node_iterator.h"
#include "Firestore/core/src/firebase/firestore/immutable/map_entry.h"
#include "Firestore/core/src/firebase/firestore/util/iterator_adaptors.h"
#include "Firestore/core/src/firebase/firestore/util/range.h"

namespace firebase {
namespace firestore {
namespace immutable {

namespace impl {

/**
 * A base class for implementing TreeSortedMap, containing types and constants
 * that don't depend upon the template parameters to the main class.
 *
 * Note that this exists as a base class rather than as just a namespace in
 * order to make it possible for users of TreeSortedMap to avoid needing to
 * declare storage for each instantiation of the template.
 */
class TreeSortedMapBase {
 public:
  /**
   * The type of size(). Note that this is not size_t specifically to save
   * space in the TreeSortedMap implementation.
   */
  typedef uint32_t size_type;

  /**
   * A predefined value indicating "not found". Similar in function to
   * std::string:npos.
   */
  static constexpr size_type npos = static_cast<size_type>(-1);
};

}  // namespace impl

/**
 * TreeSortedMap is a value type containing a map. It is immutable, but has
 * methods to efficiently create new maps that are mutations of it.
 */
template <typename K, typename V, typename C = std::less<K>>
class TreeSortedMap : public impl::TreeSortedMapBase {
 public:
  using key_comparator_type = KeyComparator<K, V, C>;

  /**
   * The type of the entries stored in the map.
   */
  using value_type = std::pair<K, V>;

  /**
   * The type of the node containing entries of value_type.
   */
  using node_type = LlrbNode<K, V>;
  using const_iterator = LlrbNodeForwardIterator<K, V, C>;
  using const_reverse_iterator = LlrbNodeReverseIterator<K, V, C>;
  using const_key_iterator = firebase::firestore::util::iterator_first<const_iterator>;

  using node_pointer = std::shared_ptr<node_type>;

  /**
   * Creates an empty TreeSortedMap.
   */
  explicit TreeSortedMap(const C& comparator = C())
      : root_(node_type::Empty()), key_comparator_(comparator) {
  }

  /**
   * Creates an TreeSortedMap containing the given entries.
   */
  TreeSortedMap(std::initializer_list<value_type> entries,
                const C& comparator = C())
      : root_(), key_comparator_(comparator) {
    node_pointer root = node_type::Empty();
    for (auto&& entry : entries) {
      root = wrap(root->insert(entry.first, entry.second));
    }
    root_ = root;
  }

  /**
   * Creates a new map identical to this one, but with a key-value pair added or
   * updated.
   *
   * @param key The key to insert/update.
   * @param value The value to associate with the key.
   * @return A new dictionary with the added/updated value.
   */
  TreeSortedMap insert(const K& key, const V& value) const {
    if (&key) {}
    if (&value) {}
  }

  /**
   * Creates a new map identical to this one, but with a key removed from it.
   *
   * @param key The key to remove.
   * @return A new dictionary without that value.
   */
  TreeSortedMap erase(const K& key) const {
    if (&key) {}
    return *this;
  }

  /**
   * Finds a value in the map.
   *
   * @param key The key to look up.
   * @return An iterator pointing to the entry containing the key, or end() if
   *     not found.
   */
//  const_iterator find(const K& key) const {
//    if (&key) {}
//    return *this;
//  }

//  size_type find_index(const K& key) const {
//  }

  /** Returns true if the map contains no elements. */
  bool empty() const {
    return size() == 0;
  }

  /** Returns the number of items in this map. */
  size_type size() const {
    return root_->size();
  }

  /**
   * Returns of a view of this TreeSortedMap containing just the keys that
   * have been inserted.
   */
  const util::range<const_key_iterator> keys() const {
    auto keys_begin = util::make_iterator_first(begin());
    auto keys_end = util::make_iterator_first(end());
    return util::make_range(keys_begin, keys_end);
  }

  /**
   * Returns of a view of this TreeSortedMap containing just the keys that
   * have been inserted whose values are greater than or equal to the given key.
   */
  const util::range<const_key_iterator> keys_from(const K& key) const {
    auto keys_begin = util::make_iterator_first(LowerBound(key));
    auto keys_end = util::make_iterator_first(end());
    return util::make_range(keys_begin, keys_end);
  }

  /**
   * Returns of a view of this TreeSortedMap containing just the keys that
   * have been inserted whose values are greater than or equal to the given
   * begin_key and less than or equal to the given end_key.
   */
//  const util::range<const_key_iterator> keys_in(const K& begin_key,
//                                                const K& end_key) const {
//    auto keys_begin = util::make_iterator_first(LowerBound(begin_key));
//    auto keys_end = util::make_iterator_first(LowerBound(end_key));
//    return util::make_range(keys_begin, keys_end);
//  }

  /**
   * Returns of a view of this TreeSortedMap containing just the keys that
   * have been inserted in reverse order.
   */
  const util::range<std::reverse_iterator<const_key_iterator>> reverse_keys()
      const {
    auto keys_begin = util::make_iterator_first(rbegin());
    auto keys_end = util::make_iterator_first(rend());
    return util::make_range(keys_begin, keys_end);
  }

  /**
   * Returns a reverse ordered view of this TreeSortedMap whose keys are
   * greater than or equal to the given key (in the reverse ordering).
   */
//  const util::range<std::reverse_iterator<const_key_iterator>>
//  reverse_keys_from(const K& key) const {
//    auto keys_begin = util::make_iterator_first(begin());
//
//    // LowerBound returns an iterator pointing to the first element that is not
//    // less than the key. However a reverse iterator wants to be constructed
//    // from an iterator past the starting point. Therefore, keep advancing
//    // the found iterator while the key is equal.
//    auto found = LowerBound(key);
//    while (found != end() && !key_comparator_(key, *found)) {
//      ++found;
//    }
//
//    auto keys_end = util::make_iterator_first(found);
//    return util::make_reverse_range(keys_begin, keys_end);
//  }

  const node_pointer& root() const {
    return root_;
  }

  /**
   * Returns an iterator pointing to the first entry in the map. If there are
   * no entries in the map, begin() == end().
   */
  const_iterator begin() const {
    return const_iterator::Begin(root_.get());
  }

  /**
   * Returns an iterator pointing past the last entry in the map.
   */
  const_iterator end() const {
    return const_iterator::End(root_.get());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator::Begin(root_.get());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator::End(root_.get());
  }

  util::range<const_reverse_iterator> reverse() const {
    return util::make_range(rbegin(), rend());
  }

 private:
  TreeSortedMap(const node_pointer& root,
                const key_comparator_type& key_comparator) noexcept
      : root_(root), key_comparator_(key_comparator) {
  }

  const_iterator LowerBound(const K& key) {
    return const_iterator::LowerBound(root_, key, key_comparator_);
  }

//  TreeSortedMap wrap(const node_pointer& root) const noexcept {
//    return TreeSortedMap(root, key_comparator_);
//  }

//  const_iterator LowerBound(const K& key) const {
//    return std::lower_bound(begin(), end(), key, key_comparator_);
//  }

  node_pointer root_;
  key_comparator_type key_comparator_;
};

}  // namespace immutable
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_TREE_SORTED_MAP_H_
