#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "kv_storage.hpp"

class KVStorageStressTest : public testing::Test {
 protected:
  KVStorageStressTest() {
    std::vector<std::tuple<std::string, std::string, uint32_t>> data_;
    data_.reserve(1'000);

    for (int i = 0; i < 1'000; ++i) {
      data_.emplace_back("key" + std::to_string(i), "value" + std::to_string(i),
                         0);
    }

    storage_ = std::make_unique<KVStorage<std::chrono::steady_clock>>(data_);
  }

  std::unique_ptr<KVStorage<std::chrono::steady_clock>> storage_;
};

TEST_F(KVStorageStressTest, Get) {
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1'000; ++i) {
    storage_->get("key" + std::to_string(i));
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "1'000 get operations —— " << duration.count() << " microseconds"
            << std::endl;

  EXPECT_LT(duration.count(), 1'000);
}

TEST_F(KVStorageStressTest, GetManySorted) {
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1'000; i += 100) {
    storage_->getManySorted("key" + std::to_string(i), 100);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "10 getManySorted(key, 100) operations —— " << duration.count()
            << " microseconds" << std::endl;

  EXPECT_LT(duration.count(), 1'000);
}

TEST_F(KVStorageStressTest, MixedWorkload) {
  std::vector<int> indices(1'000);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(),
               std::mt19937{std::random_device{}()});

  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> prob(0.0, 1.0);
  std::uniform_int_distribution<int> key_dist(0, 999);
  std::uniform_int_distribution<int> count_dist(1, 10);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 10'000; ++i) {
    double p = prob(rng);
    std::string key = "key" + std::to_string(key_dist(rng));

    if (p < 0.95) {
      if (prob(rng) < 0.5) {
        storage_->get(key);
      } else {
        storage_->getManySorted(key, count_dist(rng));
      }
    } else {
      double modification_type = prob(rng);
      if (modification_type < 1.0 / 3.0) {
        storage_->set(key, "updated" + std::to_string(i), 0);
      } else if (modification_type < 2.0 / 3.0) {
        storage_->remove(key);
      } else {
        storage_->removeOneExpiredEntry();
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "10'000 operations, mixed random workload —— "
            << duration.count() << " ms" << std::endl;

  EXPECT_LT(duration.count(), 100);
}
