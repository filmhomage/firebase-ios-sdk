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

#include "Firestore/core/src/firebase/firestore/immutable/array_sorted_map.h"

#include <numeric>
#include <random>

#include "Firestore/core/test/firebase/firestore/immutable/testing.h"
#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace immutable {

typedef ArraySortedMap<int, int> IntMap;
constexpr IntMap::size_type kFixedSize = IntMap::kFixedSize;

/**
 * Creates an ArraySortedMap by inserting a pair for each value in the vector.
 * Each pair will have the same key and value.
 */
IntMap ToMap(const std::vector<int>& values) {
  IntMap result;
  for (auto&& value : values) {
    result = result.insert(value, value);
  }
  return result;
}

// TODO(wilhuff): ReverseTraversal

TEST(ArraySortedMap, SearchForSpecificKey) {
  IntMap map{{1, 3}, {2, 4}};

  ASSERT_TRUE(Found(map, 1, 3));
  ASSERT_TRUE(Found(map, 2, 4));
  ASSERT_TRUE(NotFound(map, 3));
}

TEST(ArraySortedMap, RemoveKeyValuePair) {
  IntMap map{{1, 3}, {2, 4}};

  IntMap new_map = map.erase(1);
  ASSERT_TRUE(Found(new_map, 2, 4));
  ASSERT_TRUE(NotFound(new_map, 1));

  // Make sure the original one is not mutated
  ASSERT_TRUE(Found(map, 1, 3));
  ASSERT_TRUE(Found(map, 2, 4));
}

TEST(ArraySortedMap, MoreRemovals) {
  IntMap map = IntMap()
                   .insert(1, 1)
                   .insert(50, 50)
                   .insert(3, 3)
                   .insert(4, 4)
                   .insert(7, 7)
                   .insert(9, 9)
                   .insert(1, 20)
                   .insert(18, 18)
                   .insert(3, 2)
                   .insert(4, 71)
                   .insert(7, 42)
                   .insert(9, 88);

  ASSERT_TRUE(Found(map, 7, 42));
  ASSERT_TRUE(Found(map, 3, 2));
  ASSERT_TRUE(Found(map, 1, 20));

  IntMap s1 = map.erase(7);
  IntMap s2 = map.erase(3);
  IntMap s3 = map.erase(1);

  ASSERT_TRUE(NotFound(s1, 7));
  ASSERT_TRUE(Found(s1, 3, 2));
  ASSERT_TRUE(Found(s1, 1, 20));

  ASSERT_TRUE(Found(s2, 7, 42));
  ASSERT_TRUE(NotFound(s2, 3));
  ASSERT_TRUE(Found(s2, 1, 20));

  ASSERT_TRUE(Found(s3, 7, 42));
  ASSERT_TRUE(Found(s3, 3, 2));
  ASSERT_TRUE(NotFound(s3, 1));
}

TEST(ArraySortedMap, RemovesMiddle) {
  IntMap map{{1, 1}, {2, 2}, {3, 3}};
  ASSERT_TRUE(Found(map, 1, 1));
  ASSERT_TRUE(Found(map, 2, 2));
  ASSERT_TRUE(Found(map, 3, 3));

  IntMap s1 = map.erase(2);
  ASSERT_TRUE(Found(s1, 1, 1));
  ASSERT_TRUE(NotFound(s1, 2));
  ASSERT_TRUE(Found(s1, 3, 3));
}

TEST(ArraySortedMap, Increasing) {
  auto total = static_cast<int>(kFixedSize);
  IntMap map;

  for (int i = 0; i < total; i++) {
    map = map.insert(i, i);
  }
  ASSERT_EQ(kFixedSize, map.size());

  for (int i = 0; i < total; i++) {
    map = map.erase(i);
  }
  ASSERT_EQ(0u, map.size());
}

TEST(ArraySortedMap, Override) {
  IntMap map = IntMap().insert(10, 10).insert(10, 8);

  ASSERT_TRUE(Found(map, 10, 8));
  ASSERT_FALSE(Found(map, 10, 10));
}

TEST(ArraySortedMap, ChecksSize) {
  std::vector<int> to_insert = Sequence(kFixedSize);
  IntMap map = ToMap(to_insert);

  // Replacing an existing entry should not hit increase size
  map = map.insert(5, 10);

  int next = kFixedSize;
  ASSERT_DEATH_IF_SUPPORTED(map.insert(next, next), "new_size <= fixed_size");
}

TEST(ArraySortedMap, Empty) {
  IntMap map = IntMap().insert(10, 10).erase(10);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(0u, map.size());
  EXPECT_TRUE(NotFound(map, 1));
  EXPECT_TRUE(NotFound(map, 10));
}

TEST(ArraySortedMap, EmptyGet) {
  IntMap map;
  EXPECT_TRUE(NotFound(map, 10));
}

TEST(ArraySortedMap, EmptySize) {
  IntMap map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(0u, map.size());
}

TEST(ArraySortedMap, EmptyRemoval) {
  IntMap map;
  IntMap new_map = map.erase(1);
  EXPECT_TRUE(new_map.empty());
  EXPECT_EQ(0u, new_map.size());
  EXPECT_TRUE(NotFound(new_map, 1));
}

TEST(ArraySortedMap, ReverseTraversal) {
  IntMap map =
      IntMap().insert(1, 1).insert(5, 5).insert(3, 3).insert(2, 2).insert(4, 4);

  auto expected = Pairs(Sequence(5, 0, -1));
  EXPECT_SEQ_EQ(expected, map.reverse());
}

TEST(ArraySortedMap, InsertionAndRemovalOfMaxItems) {
  auto expected_size = kFixedSize;
  int n = static_cast<int>(expected_size);
  std::vector<int> to_insert = Shuffled(Sequence(n));
  std::vector<int> to_remove = Shuffled(to_insert);

  // Add them to the map
  IntMap map = ToMap(to_insert);
  ASSERT_EQ(expected_size, map.size())
      << "Check if all N objects are in the map";

  // check the order is correct
  ASSERT_SEQ_EQ(Pairs(Sorted(to_insert)), map);

  for (int i : to_remove) {
    map = map.erase(i);
  }
  ASSERT_EQ(0u, map.size()) << "Check we removed all of the items";
}

TEST(ArraySortedMap, BalanceProblem) {
  std::vector<int> to_insert{1, 7, 8, 5, 2, 6, 4, 0, 3};

  IntMap map = ToMap(to_insert);
  ASSERT_SEQ_EQ(Pairs(Sorted(to_insert)), map);
}

TEST(ArraySortedMap, KeyIterator) {
  std::vector<int> all = Sequence(kFixedSize);
  IntMap map = ToMap(Shuffled(all));

  IntMap::const_key_iterator begin = map.keys().begin();
  ASSERT_EQ(0, *begin);

  IntMap::const_key_iterator end = map.keys().end();
  ASSERT_EQ(all.size(), static_cast<size_t>(end - begin));

  ASSERT_SEQ_EQ(all, map.keys());
}

TEST(ArraySortedMap, ReverseKeyIterator) {
  std::vector<int> all = Sequence(kFixedSize);
  IntMap map = ToMap(Shuffled(all));

  ASSERT_SEQ_EQ(Reversed(all), map.reverse_keys());
}

TEST(ArraySortedMap, KeysFrom) {
  std::vector<int> all = Sequence(2, 42, 2);
  IntMap map = ToMap(Shuffled(all));
  ASSERT_EQ(20u, map.size());

  // Test from before keys.
  ASSERT_SEQ_EQ(all, map.keys_from(0));

  // Test from after keys.
  ASSERT_SEQ_EQ(Empty(), map.keys_from(100));

  // Test from a key in the map.
  ASSERT_SEQ_EQ(Sequence(10, 42, 2), map.keys_from(10));

  // Test from in between keys.
  ASSERT_SEQ_EQ(Sequence(12, 42, 2), map.keys_from(11));
}

TEST(ArraySortedMap, KeysIn) {
  std::vector<int> all = Sequence(2, 42, 2);
  IntMap map = ToMap(Shuffled(all));
  ASSERT_EQ(20u, map.size());

  auto Seq = [](int start, int end) { return Sequence(start, end, 2); };

  ASSERT_SEQ_EQ(Empty(), map.keys_in(0, 1));   // before to before
  ASSERT_SEQ_EQ(all, map.keys_in(0, 100))      // before to after
  ASSERT_SEQ_EQ(Seq(2, 6), map.keys_in(0, 6))  // before to in map
  ASSERT_SEQ_EQ(Seq(2, 8), map.keys_in(0, 7))  // before to in between

  ASSERT_SEQ_EQ(Empty(), map.keys_in(100, 0));    // after to before
  ASSERT_SEQ_EQ(Empty(), map.keys_in(100, 110));  // after to after
  ASSERT_SEQ_EQ(Empty(), map.keys_in(100, 6));    // after to in map
  ASSERT_SEQ_EQ(Empty(), map.keys_in(100, 7));    // after to in between

  ASSERT_SEQ_EQ(Empty(), map.keys_in(6, 0));       // in map to before
  ASSERT_SEQ_EQ(Seq(6, 42), map.keys_in(6, 100));  // in map to after
  ASSERT_SEQ_EQ(Seq(6, 10), map.keys_in(6, 10));   // in map to in map
  ASSERT_SEQ_EQ(Seq(6, 12), map.keys_in(6, 11));   // in map to in between

  ASSERT_SEQ_EQ(Empty(), map.keys_in(7, 0));       // in between to before
  ASSERT_SEQ_EQ(Seq(8, 42), map.keys_in(7, 100));  // in between to after
  ASSERT_SEQ_EQ(Seq(8, 10), map.keys_in(7, 10));   // in between to key in map
  ASSERT_SEQ_EQ(Seq(8, 12), map.keys_in(7, 11));   // in between to in between
}

TEST(ArraySortedMap, ReverseKeysFrom) {
  std::vector<int> all = Sequence(2, 42, 2);
  std::vector<int> to_insert = Shuffled(all);
  IntMap map = ToMap(to_insert);
  ASSERT_EQ(20u, map.size());

  auto Seq = [](int start, int end) { return Sequence(start, end, -2); };

  // Test from before keys
  ASSERT_SEQ_EQ(Empty(), map.reverse_keys_from(0));

  // Test from after keys.
  ASSERT_SEQ_EQ(Seq(40, 0), map.reverse_keys_from(100));

  // Test from key in map.
  ASSERT_SEQ_EQ(Seq(10, 0), map.reverse_keys_from(10));

  // Test from in between keys.
  ASSERT_SEQ_EQ(Seq(10, 0), map.reverse_keys_from(11));
}

TEST(ArraySortedMap, FindIndex) {
  IntMap map = IntMap{{1, 1}, {3, 3}, {4, 4}, {7, 7}, {9, 9}, {50, 50}};

  ASSERT_EQ(IntMap::npos, map.find_index(0));
  ASSERT_EQ(0u, map.find_index(1));
  ASSERT_EQ(IntMap::npos, map.find_index(2));
  ASSERT_EQ(1u, map.find_index(3));
  ASSERT_EQ(2u, map.find_index(4));
  ASSERT_EQ(IntMap::npos, map.find_index(5));
  ASSERT_EQ(IntMap::npos, map.find_index(6));
  ASSERT_EQ(3u, map.find_index(7));
  ASSERT_EQ(IntMap::npos, map.find_index(8));
  ASSERT_EQ(4u, map.find_index(9));
  ASSERT_EQ(5u, map.find_index(50));
}

TEST(ArraySortedMap, AvoidsCopying) {
  IntMap map = IntMap().insert(10, 20);
  auto found = map.find(10);
  ASSERT_NE(found, map.end());
  EXPECT_EQ(20, found->second);

  // Verify that inserting something with equal keys and values just returns
  // the same underlying array.
  IntMap duped = map.insert(10, 20);
  auto duped_found = duped.find(10);

  // If everything worked correctly, the backing array should not have been
  // copied and the pointer to the entry with 10 as key should be the same.
  EXPECT_EQ(found, duped_found);
}

}  // namespace immutable
}  // namespace firestore
}  // namespace firebase
