#include <gtest/gtest.h>

#include <kv_storage.hpp>

TEST(KVStorageStressTest, HelloWorld) {
  EXPECT_STRNE("hello", "world");
  EXPECT_EQ(7 * 6, 42);
}
