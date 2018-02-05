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

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_ITERATOR_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_ITERATOR_H_

#include <iterator>
#include <stack>

#include "Firestore/core/src/firebase/firestore/immutable/llrb_node.h"
#include "Firestore/core/src/firebase/firestore/util/comparison.h"

namespace firebase {
namespace firestore {
namespace immutable {

template <typename K, typename V>
struct ForwardPolicy {
  using node_type = LlrbNode<K, V>;
  using node_pointer_type = node_type*;

  static constexpr util::ComparisonResult direction() {
    return util::ComparisonResult::Ascending;
  }

  static node_pointer_type forward(node_pointer_type node) {
    return node->left().get();
  }

  static node_pointer_type backward(node_pointer_type node) {
    return node->right().get();
  }
};

template <typename K, typename V>
struct ReversePolicy {
  using node_type = LlrbNode<K, V>;
  using node_pointer_type = node_type*;

  static constexpr util::ComparisonResult direction() {
    return util::ComparisonResult::Descending;
  }

  static node_pointer_type forward(node_pointer_type node) {
    return node->right().get();
  }

  static node_pointer_type backward(node_pointer_type node) {
    return node->left().get();
  }
};

/**
 * A bidirectional iterator for traversing LlrbNodes.
 *
 * Note: LlrbNode is an immutable tree, where insertions create new trees
 * without invalidating any of the old instances. This means the tree cannot
 * contain parent pointers and thus this iterator implementation must keep an
 * explicit stack.
 */
template <typename K, typename V, typename Policy, typename C = std::less<K>>
class LlrbNodeIterator {
 private:
  using node_type = LlrbNode<K, V>;
  using node_pointer_type = node_type*;
  using stack_type = std::stack<node_pointer_type>;

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = node_type;
  using pointer = value_type*;
  using reference = value_type&;
  using difference_type = std::ptrdiff_t;

  static LlrbNodeIterator Begin(const node_pointer_type root) {
    stack_type stack;

    node_pointer_type node = root;
    while (!node->empty()) {
      stack.push(node);
      node = Policy::backward(node);
    }

    return LlrbNodeIterator(std::move(stack), /* end= */ stack.empty());
  }

  static LlrbNodeIterator End(const node_pointer_type root) {
    stack_type stack;

    node_pointer_type node = root;
    while (!node->empty()) {
      stack.push(node);
      node = Policy::forward(node);
    }

    return LlrbNodeIterator(std::move(stack), /*end=*/ true);
  }

  static LlrbNodeIterator LowerBound(const node_pointer_type root, const K& key, const C& comparator) {
    stack_type stack;

    node_pointer_type node = root;
    while (!node->empty()) {
      stack.push(node);

      util::ComparisonResult cmp = util::Compare(root->key(), key, comparator);
      if (cmp == util::ComparisonResult::Same) {
        // Found exactly what we're looking for so we're done.
        break;

      } else if (cmp == Policy::direction()) {
        // node is less than the key (for the forward direction)
        node = Policy::forward(node);
      } else {
        node = Policy::backward(node);
      }
    }

    return LlrbNodeIterator(std::move(stack), /*end=*/root->empty());
  }

  node_pointer_type top() const {
    if (end_) {
      return node_type::Empty().get();
    } else {
      return stack_.top();
    }
  }
  pointer get() const {
    return top();
  }
  reference operator*() const {
    return *get();
  }
  pointer operator->() const {
    return get();
  }

  LlrbNodeIterator& operator++() {
    if (end_) {
      // First entry of the stack is reserved for the empty node. Cannot advance
      // past the end of the tree.
      return *this;
    }

    // Pop the stack, moving the currently pointed to node to the parent.
    node_pointer_type node = stack_.top();
    stack_.pop();

    // If the popped node has a right subtree that has to precede the parent in
    // the iteration order so push those on.
    node = Policy::forward(node);
    while (!node->empty()) {
      stack_.push(node);
      node = Policy::backward(node);
    }

    if (stack_.empty()) {
      end_ = true;
    }

    return *this;
  }

  LlrbNodeIterator& operator--() {
    if (end_) {
      end_ = false;
      return *this;
    }

    // Pop the stack, moving the currently pointed to node to the parent.
    node_pointer_type node = stack_.top();
    stack_.pop();

    // If the popped node has a right subtree that has to precede the parent in
    // the iteration order so push those on.
    node = Policy::backward(node);
    while (!node->empty()) {
      stack_.push(node);
      node = Policy::forward(node);
    }
    return *this;
  }

  LlrbNodeIterator operator++(int /*unused*/) {
    LlrbNodeIterator result = *this;
    ++*this;
    return result;
  }
  LlrbNodeIterator operator--(int /*unused*/) {
    LlrbNodeIterator result = *this;
    --*this;
    return result;
  }

  bool operator==(LlrbNodeIterator b) const {
    return get() == b.get();
  }
  bool operator!=(LlrbNodeIterator b) const {
    return get() != b.get();
  }

  friend bool operator<(LlrbNodeIterator a, LlrbNodeIterator b) {
    return a.get() < b.get();
  }
  friend bool operator>(LlrbNodeIterator a, LlrbNodeIterator b) {
    return a.get() > b.get();
  }
  friend bool operator<=(LlrbNodeIterator a, LlrbNodeIterator b) {
    return a.get() <= b.get();
  }
  friend bool operator>=(LlrbNodeIterator a, LlrbNodeIterator b) {
    return a.get() >= b.get();
  }
 private:
  LlrbNodeIterator(stack_type&& stack, bool end)
      : stack_(std::move(stack)), end_(end) {
  }

  stack_type stack_;
  bool end_;
};

template <typename K, typename V, typename C = std::less<K>>
using LlrbNodeForwardIterator = LlrbNodeIterator<K, V, ForwardPolicy<K, V>, C>;

template <typename K, typename V, typename C = std::less<K>>
using LlrbNodeReverseIterator = LlrbNodeIterator<K, V, ReversePolicy<K, V>, C>;

}
}
}

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_ITERATOR_H_
