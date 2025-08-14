#include <gtest/gtest.h>

#include <memory>

#include "kv_storage.hpp"

class KVStorageUnitTest : public testing::Test {
 protected:
  KVStorageUnitTest() {
    std::vector<std::tuple<std::string, std::string, uint32_t>> data_ = {
        {"key1", "value1", 0},
        {"key2", "value2", 1'000'000},
        {"key3", "value3", 0}};
    storage_ = std::make_unique<KVStorage<std::chrono::steady_clock>>(data_);
  }

  std::unique_ptr<KVStorage<std::chrono::steady_clock>> storage_;
};

TEST_F(KVStorageUnitTest, Get) {
  auto value = storage_->get("key1");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "value1");

  auto no_value = storage_->get("key0");
  EXPECT_FALSE(no_value.has_value());
}

TEST_F(KVStorageUnitTest, Set) {
  storage_->set("abc", "abc", 0);
  auto value = storage_->get("abc");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "abc");
}

TEST_F(KVStorageUnitTest, SetWithTtl) {
  storage_->set("abc", "abc", 1'000'000);
  auto value = storage_->get("abc");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "abc");
}

TEST_F(KVStorageUnitTest, SetLargeValue) {
  std::string large(10'000, 'x');
  storage_->set("abc", large, 0);
  auto value = storage_->get("abc");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, large);
}

TEST_F(KVStorageUnitTest, SetOverwriteValue) {
  storage_->set("abc", "first", 0);
  storage_->set("abc", "second", 0);
  auto value = storage_->get("abc");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "second");
}

TEST_F(KVStorageUnitTest, Update) {
  storage_->set("key1", "updated_value", 0);
  auto value = storage_->get("key1");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "updated_value");
}

TEST_F(KVStorageUnitTest, UpdateWithTtl) {
  storage_->set("key1", "updated_value", 1'000'000);
  auto value = storage_->get("key1");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "updated_value");
}

TEST_F(KVStorageUnitTest, Remove) {
  EXPECT_TRUE(storage_->remove("key1"));
  EXPECT_FALSE(storage_->get("key1").has_value());
  EXPECT_FALSE(storage_->remove("key1"));
}

TEST_F(KVStorageUnitTest, RemoveNothing) {
  EXPECT_FALSE(storage_->remove("abc"));
}

TEST_F(KVStorageUnitTest, GetManySortedAll) {
  auto results = storage_->getManySorted("", 10);

  EXPECT_EQ(results.size(), 3);
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_LE(results[i - 1].first, results[i].first);
  }
}

TEST_F(KVStorageUnitTest, GetManySortedFromMiddle) {
  storage_->set("key4", "key4", 0);
  storage_->set("key5", "key5", 0);
  storage_->set("key6", "key6", 0);

  auto results = storage_->getManySorted("key2", 3);

  EXPECT_EQ(results.size(), 3);
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_LE(results[i - 1].first, results[i].first);
  }
}

TEST_F(KVStorageUnitTest, Empty) {
  EXPECT_TRUE(storage_->remove("key1"));
  EXPECT_TRUE(storage_->remove("key2"));
  EXPECT_TRUE(storage_->remove("key3"));

  EXPECT_FALSE(storage_->get("abc").has_value());
  EXPECT_FALSE(storage_->remove("abc"));

  EXPECT_TRUE(storage_->getManySorted("", 10).empty());

  EXPECT_FALSE(storage_->removeOneExpiredEntry().has_value());
}
