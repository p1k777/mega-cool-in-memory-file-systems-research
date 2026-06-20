# In-memory File Systems Research

Проект исследует производительность трёх реализаций in-memory файловой системы и предоставляет воспроизводимый бенчмарк для их сравнения.

В репозитории присутствуют:

- три реализации файловой системы: `A`, `B`, `C`;
- генератор синтетической структуры файловой системы;
- генератор последовательностей операций и путей;
- C++-бенчмарк, который измеряет latency / throughput / memory usage;
- Python-скрипт для пакетного запуска независимых benchmark jobs;
- набор CSV-результатов и файл с инженерной оценкой времени прогона.

## Что делает проект

Идея проекта такая:

1. Генерируется синтетическая файловая система с параметрами глубины, ширины и заполнения.
2. Для заданного профиля нагрузки строится последовательность операций `read`, `write`, `mkdir`, `ls`, `mv`, `find`.
3. Для этой последовательности генерируются пути с выбранным распределением обращений.
4. Одна и та же подготовленная нагрузка прогоняется на одной из реализаций `A`, `B` или `C`.
5. На выходе получаем CSV с метриками производительности.

Это не файловая система уровня ОС и не FUSE-проект. Это исследовательский стенд для сравнения внутренних структур данных и стоимости типовых файловых операций.

## Реализации файловых систем

В проекте есть три реализации, все работают через общий интерфейс [`src/lib/filesystem.hpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/filesystem.hpp):

- `A` (`src/lib/A_fs`) - древовидная реализация `TreeFileSystem`.
- `B` (`src/lib/filesystem_b`) - отдельная реализация `FileSystemB`.
- `C` (`src/lib/C_fs`) - плоская hash-based реализация `FlatHashFileSystem`.

Общий интерфейс поддерживает операции:

- `op_read`
- `op_write`
- `op_mkdir`
- `op_ls`
- `op_mv`
- `op_find`
- `get_memory_usage`

## Структура репозитория

```text
.
├── CMakeLists.txt
├── README.md
├── BENCHMARK_RUNTIME_PREDICTION.md
├── scripts/
│   └── run_bench_jobs.py
├── src/
│   ├── CMakeLists.txt
│   └── lib/
│       ├── A_fs/                  # Реализация A
│       ├── filesystem_b/          # Реализация B
│       ├── C_fs/                  # Реализация C
│       ├── fs_generator/          # Генерация синтетической FS
│       ├── path_generator/        # Генерация путей для операций
│       ├── operation_generation/  # Генерация типов операций
│       ├── random/                # Вспомогательный RNG
│       └── benchmark/             # Бенчмарк и CLI runner
├── bench_results/                 # Примеры результатов
├── bench_results_full/            # Полные результаты
└── bench_results_full_3e6/        # Отдельный набор результатов
```

Ниже перечислены блоки, которые пока не оформлены в этом репозитории:

- `TODO`: директория для обучения моделей качества / регрессии.
- `TODO`: код рекомендателя, выбирающего лучшую FS по предсказанным метрикам.
- `TODO`: финальный отчёт или артефакты визуализации результатов.

## Требования

Для сборки и запуска нужны:

- `CMake >= 3.16`
- компилятор с поддержкой `C++23`
- `Python 3`
- `GTest`, если нужно собирать и запускать тесты

Python-зависимостей для `scripts/run_bench_jobs.py` сейчас не требуется: скрипт использует только стандартную библиотеку.

## Сборка

```bash
cmake -S . -B build
cmake --build build
```

После сборки основной исполняемый файл бенчмарка:

```bash
build/src/lib/benchmark/bench_run
```

Если генератор сборки у вас раскладывает бинарники иначе, ориентируйтесь на цель `bench_run` из [`src/lib/benchmark/CMakeLists.txt`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/CMakeLists.txt).

## Запуск одного benchmark job

CLI раннера реализован в [`src/lib/benchmark/bench_runner.cpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/bench_runner.cpp).

Формат запуска:

```bash
build/src/lib/benchmark/bench_run FS Repeats Ops Profile OutputCsv
```

Где:

- `FS`: `A`, `B` или `C`
- `Repeats`: число повторов для каждой точки сетки
- `Ops`: число операций в одном прогоне
- `Profile`: профиль нагрузки
- `OutputCsv`: путь к CSV-файлу результата

Пример:

```bash
build/src/lib/benchmark/bench_run A 2 2000 build_system bench_results/results_A_build_system.csv
```

Поддерживаемые профили:

- `build_system` / `build` / `bs`
- `file_manager` / `file` / `fm`
- `backup` / `bu`
- `reshaping` / `reshape` / `rs`
- `refactoring` / `ref`
- `database` / `db`
- `web_server` / `web` / `ws`

## Как работает бенчмарк

Основная логика находится в [`src/lib/benchmark/bench.hpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/bench.hpp) и [`src/lib/benchmark/bench.cpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/bench.cpp).

Бенчмарк строится так:

1. `FsGenerator` создаёт абстрактную структуру файловой системы по параметрам `D`, `W`, `F`.
2. `generate_operations(...)` строит последовательность типов операций по вероятностному профилю.
3. `UniformPathGenerator` или `ZipfPathGenerator` подбирает пути для этих операций.
4. Подготовленная синтетическая FS один раз materialize-ится в выбранную реализацию `A/B/C`.
5. Для каждого `repeat` базовая FS копируется, после чего на копии проигрывается одинаковая последовательность операций.
6. Считаются:
   - `avg_latency_us`
   - `p99_latency_us`
   - `throughput_ops_sec`
   - `memory_usage_bytes`

Важная деталь: для честного сравнения одна и та же подготовленная нагрузка используется для всех повторов внутри конкретной конфигурации.

## Сетка параметров

Текущая фиксированная сетка, используемая в `benchmark runner`:

- формы дерева:
  - `D = {2, 5, 10}`
  - `W = {5, 10, 15}`
- заполнение:
  - `F = {0.3, 0.6, 0.95}`
- профили распределения путей:
  - `uniform` с `locality = 0.0`
  - `zipf` с `locality = 0.3`, `zipf_s = 1.5`
  - `zipf` с `locality = 0.8`, `zipf_s = 2.0`

Итого одна пара `FS + profile` даёт:

```text
3 depths * 3 widths * 3 fill_factors * 3 distribution_profiles = 81 строка CSV
```

## Профили нагрузки

Профили описаны в [`src/lib/benchmark/bench.hpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/bench.hpp).

Сейчас используются такие вероятности операций:

| profile | read | write | mkdir | ls | mv | find |
|---|---:|---:|---:|---:|---:|---:|
| `build_system` | 0.50 | 0.25 | 0.10 | 0.05 | 0.05 | 0.05 |
| `file_manager` | 0.20 | 0.05 | 0.45 | 0.10 | 0.10 | 0.10 |
| `backup` | 0.10 | 0.70 | 0.00 | 0.00 | 0.20 | 0.00 |
| `reshaping` | 0.05 | 0.10 | 0.35 | 0.05 | 0.40 | 0.05 |
| `refactoring` | 0.10 | 0.10 | 0.10 | 0.10 | 0.50 | 0.10 |
| `database` | 0.55 | 0.45 | 0.00 | 0.00 | 0.00 | 0.00 |
| `web_server` | 0.80 | 0.10 | 0.00 | 0.10 | 0.00 | 0.00 |

## Формат CSV

`bench_run` пишет заголовок такого вида:

```csv
fs_type,depth,width,fill_factor,profile_name,p_read,p_write,p_mkdir,p_ls,p_mv,p_find,distribution,locality,zipf_s,operations,repeats,avg_latency_us,p99_latency_us,throughput_ops_sec,memory_usage_bytes
```

Каждая строка соответствует одной конфигурации:

- фиксированная реализация FS;
- фиксированный профиль нагрузки;
- фиксированная форма дерева;
- фиксированный профиль распределения путей;
- усреднение по `repeats`.

## Пакетный запуск

Для пакетного прогона используется [`scripts/run_bench_jobs.py`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/scripts/run_bench_jobs.py).

Скрипт запускает независимые jobs вида:

```text
одна FS + один профиль = один отдельный процесс bench_run
```

Пример:

```bash
python3 scripts/run_bench_jobs.py \
  --bench build/src/lib/benchmark/bench_run \
  --fs A B C \
  --profiles build file db web \
  --repeats 2 \
  --ops 2000 \
  --jobs 4 \
  --output-dir bench_results \
  --prefix results
```

Полезные опции:

- `--merge-csv <path>` - собрать один merged CSV из успешных jobs
- `--dry-run` - только распечатать команды

Скрипт также содержит грубую оценку ожидаемого времени выполнения job'ов. Подробности и инженерные оценки собраны в [`BENCHMARK_RUNTIME_PREDICTION.md`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/BENCHMARK_RUNTIME_PREDICTION.md).

## Тесты

В проекте есть unit-тесты как минимум для:

- `A_fs`
- `filesystem_b`
- `C_fs`
- `fs_generator`
- `operation_generation`
- `path_generator`
- `random`

Если `GTest` установлен и проект собран с `BUILD_TESTING=ON`, тесты можно запустить так:

```bash
ctest --test-dir build --output-on-failure
```

## Быстрый сценарий воспроизведения

Сборка:

```bash
cmake -S . -B build
cmake --build build
```

Один короткий прогон:

```bash
build/src/lib/benchmark/bench_run A 1 1000 db /tmp/results_A_db.csv
```

Небольшой пакетный прогон:

```bash
python3 scripts/run_bench_jobs.py \
  --bench build/src/lib/benchmark/bench_run \
  --fs A B C \
  --profiles db web build \
  --repeats 1 \
  --ops 1000 \
  --jobs 3 \
  --output-dir bench_results
```

## Ограничения и TODO

- `TODO`: описать формальную постановку исследовательского кейса.
- `TODO`: добавить ссылку на отчёт, если он существует вне репозитория.
- `TODO`: добавить раздел про обучение моделей, когда в проекте появится соответствующий код.
- `TODO`: добавить раздел про рекомендатель, когда появится исполняемая программа или библиотека.
- `TODO`: добавить визуализации результатов и методику их интерпретации.
