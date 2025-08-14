# Key-Value Storage

## Available languages

1. [Russian](README.md)
2. [English](README-en.md)

## Краткое описание

`KVStorage` — key-value хранилище с возможностью ограничения времени доступа к записям. Оптимизированно под преобладающую читающую нагрузку.

### Основные компоненты

- `KeyIndex` — хеш-таблица для быстрого доступа по ключу (`unordered_map<string, ValueMetadata>`). Хранит записи и необходимые метаданные.
- `ValueMetadata` — значение + метаданные записи (время протухания, итераторы во вторичных индексах).
- `SortedKeyIndex` — упорядоченный индекс по ключам (`set<string_view>`) для `getManySorted`.
- `TtlIndex` — индекс по времени протухания (`multimap<TimePoint, string_view>`) для `removeOneExpiredEntry`.
- `Clock` — абстракция часов для тестирования.

## Асимпотический анализ

| Метод | Временная сложность | Пояснение | Пространственная сложность | Пояснение |
|-------|---------------------|-------------------|---------------------------:|--------------------|
| `set(key, value, ttl)` | **O(log N)** | вставка в `unordered_map` O(1) амортиз.; обновление/вставка во вторичные индексы (`set`/`multimap`) — O(log N); | **O(1)** | фикс. количество вспомогательных объектов |
| `remove(key)` | **O(1) амортизированно** | поиск по ключу O(1) в среднем; удаление по сохр. итераторам во вторичных индексах — O(1) амортиз. | **O(1)** | фикс. количество вспомогательных объектов |
| `get(key)` | **O(1) в среднем** | поиск по ключу в хеш-таблице за O(1) в среднем; константное число проверок | **O(1)** | фикс. количество вспомогательных объектов |
| `getManySorted(key, count)` | **O(log N + count)** | `lower_bound` в `set` — O(log N), затем `count` операций поиска в хеш-таблице, O(1) в среднем каждая | **O(count)** | в начале метода происходит аллокация O(count) памяти, в худш. случае ничего из этого не будет использоваться для возвращаемых значений |
| `removeOneExpiredEntry()` | **O(1) амортизированно** | фикс. количество проверок — O(1); поиск в `unordered_map` за O(1) в среднем; удаление записей по сохр. итераторам — O(1) амортиз. | **O(1)** | фикс. количество вспомогательных объектов |

где N - количество хранимых в момент вызова записей.

## Оценка оверхэда памяти на запись

1. **KeyIndex (unordered_map) node**  
   - `Key` (string) = 32 B
   - `ValueMetadata`:
     - `Value` (string) = 32 B
     - `expiry` (optional<TimePoint>) = 16 B
     - two iterators (`sorted_it` + `ttl_it`) = 8 B + 8 B = 16 B  
     => `ValueMetadata` = 64 B
   - unordered_map node overhead = 16 B  
**32 + 16 + 64 = 112 B**

2. **SortedKeyIndex (set) node**
   - rb-tree node overhead = 32 B
   - `KeyView` (string_view) = 16 B  
**32 + 16 = 48 B**

3. **TtlIndex (multimap) node** — только для записей с Ttl != 0
   - rb-tree node overhead = 32 B
   - `TimePoint` (unsinged long long) (8 B) + `KeyView` (16 B) = 24 B  
**32 + 24 = 56 B**

### Итоговая оценка

- **Запись без Ttl**: `112 + 48 = 160 B`  
- **Запись с Ttl**: `112 + 48 + 56 = 216 B`

## Иструкция по сборке и запуску тестов

```bash
# склонируйте репозиторий
git clone https://github.com/pr1kol2005/kv_storage.git
cd kv_storage

# выполните сборку проекта с необходимым флагом
cmake -B build -D BUILD_TESTS=ON
cmake --build build

# запустите желаемые тесты
./bin/unit_tests
./bin/time_tests
./bin/stress_tests
```
