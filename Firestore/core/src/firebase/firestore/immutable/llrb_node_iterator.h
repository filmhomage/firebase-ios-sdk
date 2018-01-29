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

namespace firebase {
namespace firestore {
namespace immutable {

/**
 * A bidirectional iterator for traversing LlrbNodes.
 *
 * Note: LlrbNode is an immutable tree, where insertions create new trees
 * without invalidating any of the old instances. This means the tree cannot
 * contain parent pointers and thus this iterator implementation must keep an
 * explicit stack.
 */
template <typename K, typename V, typename C = std::less<K>>
class LlrbNodeIterator {
 private:
  using node_type = LlrbNode<K, V>;
  using node_pointer_type = typename node_type::pointer_type;
  using stack_type = std::stack<node_pointer_type>;

 public:
  // using iterator_category = std::bidirectional_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = node_type;
  using pointer = value_type*;
  using reference = value_type&;
  using difference_type = std::ptrdiff_t;

  static LlrbNodeIterator Begin(const node_pointer_type root) {
    stack_type stack;

    // First push the empty node so that once the stack has been exhausted
    // the empty node will remain.
    stack.push(node_type::Empty());

    node_pointer_type node = root;
    while (!node->empty()) {
      stack.push(node);
      node = node->left();
    }

    return LlrbNodeIterator(std::move(stack));
  }

  static LlrbNodeIterator End(const node_pointer_type root) {
    stack_type stack;

    node_pointer_type node = root;
    while (!node->empty()) {
      stack.push(node);
      node = node->right();
    }

    // One past the end: push one of the empty children too.
    stack.push(node_type::Empty());

    return LlrbNodeIterator(std::move(stack));
  }

  node_pointer_type top() const {
    return stack_.top();
  }
  pointer get() const {
    return top().get();
  }
  reference operator*() const {
    return *get();
  }
  pointer operator->() const {
    return get();
  }

  LlrbNodeIterator& operator++() {
    if (stack_.size() <= 1) {
      // First entry of the stack is reserved for the empty node. Cannot advance
      // past the end of the tree.
      return *this;
    }

    // Pop the stack, moving the currently pointed to node to the parent.
    node_pointer_type node = stack_.top();
    stack_.pop();

    // If the popped node has a right subtree that has to precede the parent in
    // the iteration order so push those on.
    node = node->right();
    while (!node->empty()) {
      stack_.push(node);
    }

    return *this;
  }
//  LlrbNodeIterator& operator--() {
//    return *this;
//  }
  LlrbNodeIterator operator++(int /*unused*/) {
    LlrbNodeIterator result = *this;
    ++*this;
    return result;
  }
//  LlrbNodeIterator operator--(int /*unused*/) {
//    return it_--;
//  }

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
  LlrbNodeIterator(stack_type&& stack)
      : stack_(std::move(stack)) {
  }

  stack_type stack_;
};

}
}
}

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_IMMUTABLE_LLRB_NODE_ITERATOR_H_
