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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_H_

#include <assert.h>

#include <functional>
#include <iostream>
#include <memory>
#include <utility>

namespace firebase {
namespace firestore {
namespace immutable {

/**
 * A Color of a tree node in a red-black tree.
 */
enum Color {
  Black = 0u,
  Red = 1u,

  Default = static_cast<unsigned int>(Red),
};

/** Constants and types that apply to all types of LlrbNodes. */
class LlrbNodeBase {
 public:
  // size_t is frequently 64 bits on our common platforms but there's just no
  // way we're going to build trees with that many nodes.
  typedef uint32_t size_type;

  static constexpr size_type npos = static_cast<size_type>(-1);
};

/**
 * LlrbNode is a node in a TreeSortedMap.
 */
template <typename K, typename V>
class LlrbNode : LlrbNodeBase {
 public:
  typedef LlrbNode<K, V> this_type;

  /**
   * The type of the entries stored in the map.
   */
  typedef std::pair<K, V> value_type;

  typedef std::shared_ptr<const LlrbNode<K, V>> pointer_type;

  /**
   * Returns an empty node.
   */
  static pointer_type Empty() {
    static const pointer_type empty_node =
        Wrap(LlrbNode<K, V>(K(), V(), Color::Black, 0u, nullptr, nullptr));
    return empty_node;
  }

  /**
   * Creates a new LlrbNode with the given key and value.
   */
  static pointer_type Create(const K& key, const V& value) {
    return std::make_shared<const LlrbNode<K, V>>(key, value);
  }

  static pointer_type Create(const K& key,
                             const V& value,
                             Color color,
                             const pointer_type& left,
                             const pointer_type& right) {
    return std::make_shared<const LlrbNode<K, V>>(key, value, color, left,
                                                  right);
  }

  static pointer_type Wrap(LlrbNode<K, V>&& node) {
    return std::make_shared<const LlrbNode<K, V>>(std::move(node));
  }

  LlrbNode(const K& key,
           const V& value,
           Color color = Color::Default,
           const pointer_type& left = Empty(),
           const pointer_type& right = Empty())
      : key_(key),
        value_(value),
        red_(static_cast<uint32_t>(color)),
        left_(left),
        right_(right) {
    size_ = left->size_ + 1 + right_->size_;
  }

  /** Returns a tree node with the given key-value pair set/updated. */
  template <typename Comparator = std::less<K>>
  pointer_type insert(const K& key,
                      const V& value,
                      const Comparator& comparator = Comparator()) const;

  /** Returns the number of elements at this node or beneath it in the tree. */
  size_type size() const {
    return size_;
  }

  /** Returns true if this is an empty node--a leaf node in the tree. */
  bool empty() const {
    // left_ and right_ cannot be empty if data_ is empty so we only need to
    // to check if data_ itself is empty.
    return size_ == 0;
  }

  /** Returns true if this node is red (as opposed to black). */
  bool red() const {
    return static_cast<bool>(red_);
  }

  const K& key() const {
    return key_;
  }
  const V& value() const {
    return value_;
  }
  Color color() const {
    return red_ ? Color::Red : Color::Black;
  }
  const pointer_type& left() const {
    return left_;
  }
  const pointer_type& right() const {
    return right_;
  }

 private:
  LlrbNode(const K& key,
           const V& value,
           uint32_t red,
           size_type size,
           const pointer_type& left,
           const pointer_type& right)
      : key_(key),
        value_(value),
        red_(red),
        size_(size),
        left_(left),
        right_(right) {
  }

  /** Returns the opposite color of the argument. */
  static size_type OppositeColor(size_type color) {
    return color == Color::Red ? Color::Black : Color::Red;
  }

  /** Returns a tree node with the given key-value pair set/updated. */
  template <typename Comparator = std::less<K>>
  LlrbNode<K, V> InsertImpl(const K& key,
                            const V& value,
                            const Comparator& comparator) const;

  void FixUp();

  void RotateLeft();
  void RotateRight();
  void FlipColor();

  K key_;
  V value_;

  // Store the color in the high bit of the size to save memory.
  size_type red_ : 1;
  size_type size_ : 31;

  std::shared_ptr<const LlrbNode<K, V>> left_;
  std::shared_ptr<const LlrbNode<K, V>> right_;

  friend std::ostream& operator<<(std::ostream& out,
                                  const LlrbNode<K, V>& value);
};

template <typename K, typename V>
template <typename Comparator>
typename LlrbNode<K, V>::pointer_type LlrbNode<K, V>::insert(
    const K& key, const V& value, const Comparator& comparator) const {
  LlrbNode<K, V> root = InsertImpl(key, value, comparator);
  // The root must always be black
  if (root.red()) {
    root.red_ = Color::Black;
  }
  return Wrap(std::move(root));
}

template <typename K, typename V>
template <typename Comparator>
LlrbNode<K, V> LlrbNode<K, V>::InsertImpl(const K& key,
                                          const V& value,
                                          const Comparator& comparator) const {
  if (empty()) {
    return LlrbNode<K, V>(key, value);
  }

  // Inserting is going to result in a copy but we can save some allocations by
  // creating the copy on the stack, performing fixups on the new copy and then
  // finally wrapping the result.
  LlrbNode<K, V> result(*this);

  bool descending = comparator(value, value_);
  if (descending) {
    result.left_ = Wrap(result.left_->InsertImpl(key, value, comparator));
    result.FixUp();

  } else {
    bool ascending = comparator(value_, value);
    if (ascending) {
      result.right_ = Wrap(result.right_->InsertImpl(key, value, comparator));
      result.FixUp();

    } else {
      result.value_ = value;
    }
  }
  return result;
}

template <typename K, typename V>
void LlrbNode<K, V>::FixUp() {
  size_ = left_->size() + 1 + right_->size();

  if (right_->red() && !left_->red()) {
    RotateLeft();
  }
  if (left_->red() && left_->left_->red()) {
    RotateRight();
  }
  if (left_->red() && right_->red()) {
    FlipColor();
  }
}

// Rotates left:
//
//      X              R
//    /   \          /   \
//   L     R   =>   X    RR
//        / \      / \
//       RL RR     L RL
template <typename K, typename V>
void LlrbNode<K, V>::RotateLeft() {
  pointer_type left = Create(key_, value_, Color::Red, left_, right_->left_);
  pointer_type right = right_->right_;

  // size_ remains unchanged after a rotation. Also preserve color.
  value_ = right_->value_;
  left_ = left;
  right_ = right;
}

// Rotates left:
//
//      X              L
//    /   \          /   \
//   L     R   =>   LL    X
//  / \                  / \
// LL LR                LR R
template <typename K, typename V>
void LlrbNode<K, V>::RotateRight() {
  pointer_type left = left_->left_;
  pointer_type right = Create(key_, value_, Color::Red, left_->right_, right_);

  // size_ remains unchanged after a rotation. Preserve color too.
  value_ = left_->value_;
  left_ = left;
  right_ = right;
}

template <typename K, typename V>
void LlrbNode<K, V>::FlipColor() {
  LlrbNode<K, V> new_left = *left_;
  new_left.red_ = OppositeColor(left_->red_);

  LlrbNode<K, V> new_right = *right_;
  new_right.red_ = OppositeColor(right_->red_);

  // Preserve value_ and size_
  red_ = OppositeColor(red_);
  left_ = Wrap(std::move(new_left));
  right_ = Wrap(std::move(new_right));
}

}  // namespace immutable
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_H_
