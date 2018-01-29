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

#include "Firestore/core/src/firebase/firestore/auth/user.h"

#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace auth {

TEST(User, Getter) {
  User anonymous;
  EXPECT_EQ("", anonymous.uid());
  EXPECT_FALSE(anonymous.is_authenticated());

  User signin("abc");
  EXPECT_EQ("abc", signin.uid());
  EXPECT_TRUE(signin.is_authenticated());
}

TEST(User, Comparison) {
  EXPECT_EQ(User(), User());
  EXPECT_EQ(User("abc"), User("abc"));
  EXPECT_NE(User(), User("abc"));
  EXPECT_NE(User("abc"), User("xyz"));
}

}  // namespace auth
}  // namespace firestore
}  // namespace firebase
