#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
#include <absl/container/node_hash_map.h>

#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <unordered_map>
#include <set>

template <typename Clock>
  requires requires(Clock c) {
    { c.Now() } -> std::convertible_to<uint32_t>;
  }
class KVStorage {
  using Key = std::string;
  using KeyView = std::string_view;
  using Value = std::string;
  using Duration = uint32_t;
  using Timestamp = uint32_t;

  static constexpr Timestamp kNoExpiry = 0;

  using InputEntry = std::tuple<Key, Value, Duration>;
  using OutputEntry = std::pair<Key, Value>;
  using StoredValue = std::pair<Value, Timestamp>;

  // Используем absl::node_hash_map вместо absl::flat_hash_map, чтобы
  // гарантировать стабильность указателей на ключи в хранилище.
  using StoreMap = absl::node_hash_map<Key, StoredValue>;
  using SortedSet = absl::btree_set<KeyView>;
  using TTLIndex = absl::btree_multimap<Timestamp, KeyView>;

 public:
  // Инициализирует хранилище переданным множеством записей. Размер span может
  // быть очень большим. Также принимает абстракцию часов (Clock) для
  // возможности управления временем в тестах.
  explicit KVStorage(std::span<InputEntry> entries, Clock clock = Clock())
      : clock_(std::move(clock)) {
    Timestamp now = clock_.Now();
    store_.reserve(entries.size());

    for (auto& [key, value, ttl] : entries) {
      set_impl(std::move(key), std::move(value), ttl, now);
    }
  }

  ~KVStorage() = default;

  // Присваивает по ключу кеу значение value.
  // Если ttl == 0, то время жизни записи - бесконечность, иначе запись должна
  // перестать быть доступной через ttl секунд. Безусловно обновляет ttl записи.
  void set(Key key, Value value, Duration ttl) {
    Timestamp now = clock_.Now();
    set_impl(std::move(key), std::move(value), ttl, now);
  }

  // Удаляет запись по ключу кеу.
  // Возвращает true, если запись была удалена. Если ключа не было до удаления,
  // то вернет false.
  bool remove(KeyView key) {
    auto store_it = store_.find(key);
    if (store_it == store_.end()) {
      return false;
    }

    sorted_.erase(store_it->first);

    // TODO : maybe lazy deleting ??
    Timestamp expiry = store_it->second.second;
    if (expiry != kNoExpiry) {
      auto range = ttl_index_.equal_range(expiry);
      for (auto it = range.first; it != range.second; ++it) {
        if (it->second == store_it->first) {
          ttl_index_.erase(it);
          break;
        }
      }
    }

    store_.erase(store_it);

    return true;
  }

  // Получает значение по ключу key. Если данного ключа нет, то вернет
  // std::nullopt.
  std::optional<Value> get(KeyView key) const {
    auto it = store_.find(key);
    if (it == store_.end()) {
      return std::nullopt;
    }

    Timestamp expiry = it->second.second;

    if (expiry != kNoExpiry && expiry <= clock_.Now()) {
      return std::nullopt;
    }
    return it->second.first;
  }

  // Возвращает следующие count записей начиная с key в порядке
  // лексикографической сортировки ключей.
  // Пример: ("a", "val1"), ("b", "val2"), ("d", "val3"), ("e", "val4")
  // getManySorted ("c", 2) -> ("d", "val3"), ("e", "val4")
  std::vector<OutputEntry> getManySorted(KeyView key, uint32_t count) const {
    std::vector<OutputEntry> result;
    result.reserve(count);

    Timestamp now = clock_.Now();

    auto it = sorted_.lower_bound(key);

    while (it != sorted_.end() && result.size() < count) {
      auto store_it = store_.find(*it);
      if (store_it != store_.end()) {
        Timestamp expiry = store_it->second.second;
        if (expiry == kNoExpiry || expiry > now) {
          result.emplace_back(store_it->first, store_it->second.first);
        }
      }
      ++it;
    }

    return result;
  }

  // Удаляет протухшую запись из структуры и возвращает ее.
  // Если удалять нечего, то вернет std::nullopt.
  // Если на момент вызова метода протухло несколько записей, то можно удалить
  // любую.
  std::optional<OutputEntry> removeOneExpiredEntry() {
    Timestamp now = clock_.Now();

    auto expired_it = ttl_index_.begin();
    if (expired_it == ttl_index_.end() || expired_it->first > now) {
      return std::nullopt;
    }

    KeyView key = expired_it->second;
    auto store_it = store_.find(key);

    sorted_.erase(store_it->first);
    ttl_index_.erase(expired_it);
    OutputEntry result = {store_it->first, store_it->second.first};

    store_.erase(store_it);

    return result;
  }

 private:
  Clock clock_;
  TTLIndex ttl_index_;
  SortedSet sorted_;
  StoreMap store_;

  // Добавляет запись в хранилище.
  void set_impl(Key key, Value value, Duration ttl, Timestamp now) {
  }
};
