#include <gtest/gtest.h>

#include <kv_storage.hpp>

TEST(KVStorageUnitTest, HelloWorld) {
  EXPECT_STRNE("hello", "world");
  EXPECT_EQ(7 * 6, 42);
}
