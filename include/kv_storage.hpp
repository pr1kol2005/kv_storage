#include <chrono>
#include <concepts>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

// Концепт для шаблонного параметра Clock и его member types.
template <typename C>
concept KVClock = std::default_initializable<C> && std::movable<C> && requires {
  typename C::time_point;
  typename C::duration;

  { C::now() } -> std::same_as<typename C::time_point>;
  requires std::convertible_to<std::chrono::seconds, typename C::duration>;
  {
    std::declval<typename C::time_point>() +
        std::declval<typename C::duration>()
  } -> std::same_as<typename C::time_point>;
  {
    std::declval<typename C::time_point>() <=
        std::declval<typename C::time_point>()
  } -> std::same_as<bool>;
};

template <KVClock Clock>
class KVStorage {
  using Key = std::string;
  using KeyView = std::string_view;
  using Value = std::string;

  using Duration = typename Clock::duration;
  using TimePoint = typename Clock::time_point;

  using Seconds = std::chrono::seconds;
  static constexpr Seconds kNoExpiry{0};

  using InputEntry = std::tuple<Key, Value, uint32_t>;
  using OutputEntry = std::pair<Key, Value>;

  // Важно, чтобы итераторы не инвалидировались при всех операциях над
  // хранилищем (кроме непосредственного удаления записей). Поэтому std::set и
  // std::multimap — подходящие выборы.
  using SortedKeyIndex = std::set<KeyView>;
  using TtlIndex = std::multimap<TimePoint, KeyView>;

  using SortedIterator = SortedKeyIndex::iterator;
  using TtlIterator = TtlIndex::iterator;

  struct ValueMetadata {
    Value value;
    // Храним время протухания здесь, так как 95% операций — чтение.
    // Иначе пришлось бы каждый раз разыменовывать итератор в TtlIndex,
    // что хуже для cache locality.
    // ttl == 0 <=> expiry == std::nullopt.
    std::optional<TimePoint> expiry;
    // Храним итераторы на set и multimap, чтобы использовать их для
    // амортизированного O(1) удаления.
    SortedIterator sorted_it;
    // Невалиден, если expiry == std::nullopt.
    TtlIterator ttl_it;

    ValueMetadata(Value value, std::optional<TimePoint> expiry,
                  SortedIterator sorted_it, TtlIterator ttl_it)
        : value(std::move(value)),
          expiry(expiry),
          sorted_it(sorted_it),
          ttl_it(ttl_it) {}
  };

  // Кастомный хэш для поддержки heterogenous lookup.
  struct string_hash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const { return hash_type{}(str); }
    std::size_t operator()(std::string_view str) const {
      return hash_type{}(str);
    }
    std::size_t operator()(const std::string& str) const {
      return hash_type{}(str);
    }
  };

  // Важно чтобы указатели не инвалидировались при любых операциях, чтобы
  // string_view в других контейнерах оставались валидными. Поэтому
  // unordered_map — подходящий выбор.
  using KeyIndex =
      std::unordered_map<Key, ValueMetadata, string_hash, std::equal_to<>>;

 public:
  // Инициализирует хранилище переданным множеством записей. Размер span может
  // быть очень большим. Также принимает абстракцию часов (Clock) для
  // возможности управления временем в тестах.
  explicit KVStorage(std::span<InputEntry> entries, Clock clock = Clock())
      : clock_(std::move(clock)) {
    // Отсчет time to live должен начаться с момента вызова конструктора для
    // всех записей из span.
    TimePoint now = Clock::now();

    // Предотвращаем лишние вызовы rehash.
    key_index_.reserve(entries.size());

    for (auto& [key, value, ttl] : entries) {
      set_impl(std::move(key), std::move(value), static_cast<Seconds>(ttl),
               now);
    }
  }

  ~KVStorage() = default;

  // Присваивает по ключу кеу значение value.
  // Если ttl == 0, то время жизни записи - бесконечность, иначе запись должна
  // перестать быть доступной через ttl секунд. Безусловно обновляет ttl записи.
  // O(logN) time complexity.
  void set(Key key, Value value, uint32_t ttl) {
    TimePoint now = Clock::now();

    set_impl(std::move(key), std::move(value), static_cast<Seconds>(ttl), now);
  }

  // Удаляет запись по ключу кеу.
  // Возвращает true, если запись была удалена. Если ключа не было до удаления,
  // то вернет false.
  // amortized O(1) time complexity.
  bool remove(KeyView key) {
    auto entry_it = key_index_.find(key);
    if (entry_it == key_index_.end()) {
      return false;
    }

    sorted_index_.erase(entry_it->second.sorted_it);
    if (entry_it->second.expiry.has_value()) {
      ttl_index_.erase(entry_it->second.ttl_it);
    }
    key_index_.erase(entry_it);

    return true;
  }

  // Получает значение по ключу key. Если данного ключа нет, то вернет
  // std::nullopt.
  // average-case O(1) time complexity.
  std::optional<Value> get(KeyView key) const {
    auto entry_it = key_index_.find(key);

    if (entry_it == key_index_.end()) {
      return std::nullopt;
    }

    auto expiry = entry_it->second.expiry;
    if (expiry.has_value() && expiry <= Clock::now()) {
      return std::nullopt;
    }

    return entry_it->second.value;
  }

  // Возвращает следующие count записей начиная с key в порядке
  // лексикографической сортировки ключей.
  // Пример: ("a", "val1"), ("b", "val2"), ("d", "val3"), ("e", "val4")
  // getManySorted ("c", 2) -> ("d", "val3"), ("e", "val4").
  // O(logN + count) time complexity.
  std::vector<OutputEntry> getManySorted(KeyView key, uint32_t count) const {
    std::vector<OutputEntry> result;
    result.reserve(count);

    TimePoint now = Clock::now();

    auto first_it = sorted_index_.lower_bound(key);

    while (first_it != sorted_index_.end() && result.size() < count) {
      auto entry_it = key_index_.find(*first_it);

      auto expiry = entry_it->second.expiry;
      if (!expiry.has_value() || !(expiry <= now)) {
        result.emplace_back(entry_it->first, entry_it->second.value);
      }

      ++first_it;
    }

    return result;
  }

  // Удаляет протухшую запись из структуры и возвращает ее.
  // Если удалять нечего, то вернет std::nullopt.
  // Если на момент вызова метода протухло несколько записей, то можно удалить
  // любую.
  // amortized O(1) time complexity.
  std::optional<OutputEntry> removeOneExpiredEntry() {
    auto expired_it = ttl_index_.begin();
    if (expired_it == ttl_index_.end() ||
        !(expired_it->first <= Clock::now())) {
      return std::nullopt;
    }

    auto entry_it = key_index_.find(expired_it->second);

    sorted_index_.erase(entry_it->second.sorted_it);
    ttl_index_.erase(entry_it->second.ttl_it);

    auto node_handle = key_index_.extract(entry_it);

    return std::make_optional<OutputEntry>(std::move(node_handle.key()), std::move(node_handle.mapped().value));
  }

 private:
  Clock clock_;
  TtlIndex ttl_index_;
  SortedKeyIndex sorted_index_;
  KeyIndex key_index_;

  // Добавляет запись в хранилище.
  // O(logN) time complexity.
  void set_impl(Key key, Value value, Seconds ttl, TimePoint now) {
    std::optional<TimePoint> new_expiry =
        (ttl == kNoExpiry) ? std::nullopt : std::make_optional<TimePoint>(now + static_cast<Duration>(ttl));

    auto [entry_it, inserted] = key_index_.try_emplace(
        std::move(key), std::move(value), new_expiry,
        sorted_index_.end(), ttl_index_.end());

    if (inserted) {
        entry_it->second.sorted_it = sorted_index_.emplace(entry_it->first).first;
        if (new_expiry.has_value()) {
          entry_it->second.ttl_it = ttl_index_.emplace(new_expiry.value(), entry_it->first);
        }
    }

    if (!inserted) {
      entry_it->second.value = std::move(value);

      auto old_expiry = entry_it->second.expiry;
      entry_it->second.expiry = new_expiry;

      if (old_expiry.has_value()) {
        ttl_index_.erase(entry_it->second.ttl_it);
      }

      if (new_expiry.has_value()) {
        entry_it->second.ttl_it = ttl_index_.emplace(new_expiry.value(), entry_it->first);
      } else {
        entry_it->second.ttl_it = ttl_index_.end();
      }
    }
  }
};
