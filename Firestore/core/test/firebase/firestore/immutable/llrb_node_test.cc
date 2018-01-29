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

#include "Firestore/core/src/firebase/firestore/immutable/llrb_node_iterator.h"

#include <time.h>

#include <random>
#include <unordered_set>

#include "Firestore/core/src/firebase/firestore/immutable/llrb_node.h"
#include "Firestore/core/src/firebase/firestore/util/range.h"
#include "Firestore/core/src/firebase/firestore/util/secure_random.h"
#include "Firestore/core/test/firebase/firestore/immutable/testing.h"
#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace immutable {

using IntNode = LlrbNode<int, int>;
using IntNodeIterator = LlrbNodeIterator<int, int>;

IntNodeIterator Begin(IntNode::pointer_type node) {
  return IntNodeIterator::Begin(node);
}

IntNodeIterator End(IntNode::pointer_type node) {
  return IntNodeIterator::End(node);
}

/**
 * Creates an ArraySortedMap by inserting a pair for each value in the vector.
 * Each pair will have the same key and value.
 */
IntNode::pointer_type ToTree(const std::vector<int>& values) {
  IntNode::pointer_type result = IntNode::Empty();
  for (auto&& value : values) {
    result = result->insert(value, value);
  }
  return result;
}

TEST(LlrbNode, PropertiesForEmpty) {
  IntNode::pointer_type empty = IntNode::Empty();
  EXPECT_TRUE(empty->empty());
  EXPECT_EQ(0, empty->value());
  EXPECT_EQ(Color::Black, empty->color());
  EXPECT_FALSE(empty->red());
  EXPECT_EQ(nullptr, empty->left());
  EXPECT_EQ(nullptr, empty->right());
}

TEST(LlrbNode, PropertiesForNonEmpty) {
  IntNode::pointer_type empty = IntNode::Empty();

  IntNode::pointer_type node = IntNode::Create(1, 1);
  EXPECT_FALSE(node->empty());
  EXPECT_EQ(1, node->value());
  EXPECT_EQ(Color::Red, node->color());
  EXPECT_TRUE(node->red());
  EXPECT_EQ(empty, node->left());
  EXPECT_EQ(empty, node->right());
}

TEST(LlrbNode, InsertFromEmpty) {
  IntNode::pointer_type empty = IntNode::Empty();
  IntNode::pointer_type root = empty->insert(1, 1);
  EXPECT_FALSE(root->empty());
  EXPECT_EQ(1, root->value());
  EXPECT_EQ(Color::Black, root->color());
}

TEST(LlrbNode, RotatesLeft) {
  IntNode::pointer_type root = IntNode::Empty();
  root = root->insert(1, 1);
  root = root->insert(2, 2);

  EXPECT_EQ(2, root->value());
}

TEST(LlrbNode, RotatesRight) {
  IntNode::pointer_type root = IntNode::Empty();
  root = root->insert(3, 3);
  EXPECT_EQ(3, root->value());

  root = root->insert(2, 2);
  EXPECT_EQ(3, root->value());

  root = root->insert(1, 1);
  EXPECT_EQ(2, root->value());
  EXPECT_EQ(1, root->left()->value());
  EXPECT_EQ(3, root->right()->value());
}

TEST(LlrbNode, RotatesRightAndMaintainsColorInvariants) {
  IntNode::pointer_type root = IntNode::Empty();
  EXPECT_EQ(Color::Black, root->color());
  EXPECT_EQ(nullptr, root->left());   // Implicitly Color::Black
  EXPECT_EQ(nullptr, root->right());  // Implicitly Color::Black

  // root node, with two empty children
  root = root->insert(3, 3);
  EXPECT_EQ(Color::Black, root->color());
  EXPECT_EQ(Color::Black, root->left()->color());
  EXPECT_EQ(Color::Black, root->right()->color());

  // insert predecessor, leans left, no rotation
  root = root->insert(2, 2);
  EXPECT_EQ(Color::Black, root->color());
  EXPECT_EQ(Color::Red, root->left()->color());
  EXPECT_EQ(Color::Black, root->right()->color());

  EXPECT_EQ(Color::Black, root->left()->left()->color());

  // insert predecessor, rotation required
  root = root->insert(1, 1);
  EXPECT_EQ(2, root->value());
  EXPECT_EQ(Color::Black, root->color());
  EXPECT_EQ(Color::Black, root->left()->color());
  EXPECT_EQ(Color::Black, root->right()->color());
}

TEST(LlrbNode, Size) {
  std::mt19937 rand;
  std::uniform_int_distribution<int> dist(0, 999);

  // The random number sequence can generate duplicates, so the expected size
  // won't necessarily depend upon `i` in the loop.
  std::unordered_set<int> expected;

  IntNode::pointer_type root = IntNode::Empty();
  for (int i = 0; i < 100; ++i) {
    int value = dist(rand);

    expected.insert(value);

    root = root->insert(value, value);
    EXPECT_EQ(expected.size(), root->size());
  }
}

TEST(LlrbNodeIterator, BeginEndEmpty) {
  IntNode::pointer_type empty = IntNode::Empty();
  IntNodeIterator begin = Begin(empty);
  IntNodeIterator end = End(empty);
  ASSERT_EQ(IntNode::Empty().get(), end.get());
  ASSERT_EQ(begin, end);
}

TEST(LlrbNodeIterator, BeginEndOne) {
  IntNode::pointer_type node = ToTree(Sequence(1));
  IntNodeIterator begin = Begin(node);
  IntNodeIterator end = End(node);
  ASSERT_EQ(IntNode::Empty().get(), end.get());

  ASSERT_NE(begin, end);
  ASSERT_EQ(0, begin->key());

  ++begin;
  ASSERT_EQ(IntNode::Empty().get(), begin.get());
  ASSERT_EQ(begin, end);
}

TEST(LlrbNodeIterator, Increments) {
  std::vector<int> to_insert = Sequence(3);
  IntNode::pointer_type node = ToTree(to_insert);
  IntNodeIterator begin = Begin(node);
  IntNodeIterator end = End(node);

  std::vector<IntNode> result = Append(util::make_range(begin, end));
  ASSERT_EQ(Pairs(to_insert), result);
}

}  // namespace immutable
}  // namespace firestore
}  // namespace firebase
