#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>

#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <vector>

template <typename Clock>
class KVStorage {
 public:
  // Инициализирует хранилище переданным множеством записей. Размер span может
  // быть очень большим. Также принимает абстракцию часов (Clock) для
  // возможности управления временем в тестах.
  explicit KVStorage(
      std::span<std::tuple<std::string /*key*/, std::string /*value*/,
                           uint32_t /*ttl*/>>
          entries,
      Clock clock = Clock()) {
    // TODO : implement me
  }

  ~KVStorage() {
    // TODO: implement me
  }

  // Присваивает по ключу кеу значение value.
  // Если ttl == 0, то время жизни записи - бесконечность, иначе запись должна
  // перестать быть доступной через ttl секунд. Безусловно обновляет ttl записи.
  void set(std::string key, std::string value, uint32_t ttl) {
    // TODO: implement me
  }

  // Удаляет запись по ключу кеу.
  // Возвращает true, если запись была удалена. Если ключа не было до удаления,
  // то вернет false.
  bool remove(std::string_view key) {
    // TODO : implement me
    return false;
  }

  // Получает значение по ключу key. Если данного ключа нет, то вернет
  // std::nullopt.
  std::optional<std::string> get(std::string_view key) const {
    // TODO: implement me
    return std::nullopt;
  }

  // Возвращает следующие count записей начиная с key в порядке
  // лексикографической сортировки ключей.
  // Пример: ("a", "val1"), ("b", "val2"), ("d", "val3"), ("e", "val4")
  // getManySorted ("c", 2) -> ("d", "val3"), ("e", "val4")
  std::vector<std::pair<std::string, std::string>> getManySorted(
      std::string_view key, uint32_t count) const {
    // TODO: implement me
    return {};
  }

  // Удаляет протухшую запись из структуры и возвращает ее.
  // Если удалять нечего, то вернет std: :nullopt.
  // Если на момент вызова метода протухло несколько записей, то можно удалить
  // любую.
  std::optional<std::pair<std::string, std::string>> removeOneExpiredEntry() {
    // TODO: implement me
    return std::nullopt;
  }

 private:
  // TODO: implementation
};