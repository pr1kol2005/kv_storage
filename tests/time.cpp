#include <gtest/gtest.h>

#include <memory>

#include "kv_storage.hpp"

class ManualClock {
 public:
  using time_point = std::chrono::steady_clock::time_point;
  using duration = std::chrono::seconds;

  ManualClock() = default;

  static time_point now() { return *shared_time_; }

  void advance(std::chrono::seconds seconds) { *shared_time_ += seconds; }

 private:
  inline static std::shared_ptr<time_point> shared_time_{
      std::make_shared<time_point>(std::chrono::steady_clock::now())};
};

class KVStorageTimeTest : public testing::Test {
 protected:
  KVStorageTimeTest() {
    std::vector<std::tuple<std::string, std::string, uint32_t>> data_ = {
        {"infinite", "value", 0},
        {"short", "value", 10},
        {"long", "value", 1'000}};
    storage_ = std::make_unique<KVStorage<ManualClock>>(data_, clock_);
  }

  ManualClock clock_;
  std::unique_ptr<KVStorage<ManualClock>> storage_;
};

TEST_F(KVStorageTimeTest, Expiration) {
  EXPECT_TRUE(storage_->get("short").has_value());
  EXPECT_TRUE(storage_->get("long").has_value());
  EXPECT_TRUE(storage_->get("infinite").has_value());

  clock_.advance(std::chrono::seconds(11));

  EXPECT_FALSE(storage_->get("short").has_value());
  EXPECT_TRUE(storage_->get("long").has_value());
  EXPECT_TRUE(storage_->get("infinite").has_value());

  clock_.advance(std::chrono::seconds(1'000));

  EXPECT_FALSE(storage_->get("short").has_value());
  EXPECT_FALSE(storage_->get("long").has_value());
  EXPECT_TRUE(storage_->get("infinite").has_value());
}

TEST_F(KVStorageTimeTest, ExpirationRightOnTime) {
  clock_.advance(std::chrono::seconds(10));
  EXPECT_FALSE(storage_->get("short").has_value());
}

TEST_F(KVStorageTimeTest, RemoveExpiredEntry) {
  clock_.advance(std::chrono::seconds(11));

  auto expired = storage_->removeOneExpiredEntry();
  ASSERT_TRUE(expired.has_value());
  EXPECT_EQ(expired->first, "short");
  EXPECT_EQ(expired->second, "value");

  auto no_expired = storage_->removeOneExpiredEntry();
  EXPECT_FALSE(no_expired.has_value());

  clock_.advance(std::chrono::seconds(1'000));

  auto expired2 = storage_->removeOneExpiredEntry();
  ASSERT_TRUE(expired2.has_value());
  EXPECT_EQ(expired2->first, "long");
  EXPECT_EQ(expired->second, "value");

  EXPECT_TRUE(storage_->get("infinite").has_value());
}

TEST_F(KVStorageTimeTest, ExtendTtl) {
  storage_->set("short", "abc", 1'000);

  clock_.advance(std::chrono::seconds(11));

  auto value = storage_->get("short");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "abc");
}

TEST_F(KVStorageTimeTest, ExtendTtlToInfinity) {
  storage_->set("short", "abc", 0);

  clock_.advance(std::chrono::seconds(10'000));

  auto value = storage_->get("short");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "abc");
}

TEST_F(KVStorageTimeTest, GetManySortedSkipsExpired) {
  clock_.advance(std::chrono::seconds(11));

  auto results = storage_->getManySorted("", 10);

  for (const auto& [key, value] : results) {
    EXPECT_NE(key, "short");
  }
}
