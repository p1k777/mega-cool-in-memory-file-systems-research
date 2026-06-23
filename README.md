# File Systems Research

Проект исследует производительность трёх реализаций файловой системы, предоставляет воспроизводимый C++-бенчмарк для их сравнения и содержит ML-артефакты для анализа и рекомендации конфигураций.

В репозитории присутствуют:

- три реализации файловой системы: `A`, `B`, `C`;
- генератор синтетической структуры файловой системы;
- генератор последовательностей операций и путей;
- C++-бенчмарк, который измеряет latency / throughput / memory usage;
- Python-скрипт для пакетного запуска независимых benchmark jobs;
- локальный веб-рекомендатель на основе обученных Random Forest-моделей;
- ноутбуки и артефакты для анализа benchmark-результатов;
- Typst-исходники и PDF-версии исследовательского отчёта.

## Что делает проект

Идея проекта такая:

1. Генерируется синтетическая файловая система с параметрами глубины, ширины и заполнения.
2. Для заданного профиля нагрузки строится последовательность операций `read`, `write`, `mkdir`, `ls`, `mv`, `find`.
3. Для этой последовательности генерируются пути с выбранным распределением обращений.
4. Одна и та же подготовленная нагрузка прогоняется на одной из реализаций `A`, `B` или `C`.
5. На выходе получаем CSV с метриками производительности.

Дальше эти данные используются в двух соседних слоях:

- `recommendation/` использует уже обученные модели, чтобы оценивать ожидаемые метрики и предлагать реализацию под выбранный приоритет.
- `ml_model/` хранит результаты экспериментов, ноутбуки и материалы для анализа качества моделей.

# Рекомендатель

Локальный веб-интерфейс использует обученные Random Forest-модели для прогноза метрик систем A, B и C. В рекомендации можно задать приоритет памяти, задержки или пропускной способности. Стратегия задержки учитывает среднюю задержку с весом 60%, p99 — 30%, память и пропускную способность — по 5%. В остальных специализированных стратегиях приоритетная метрика получает вес 70%, остальные — по 10%; сбалансированная стратегия используется по умолчанию. Итоговый балл рассчитывается взвешенным геометрическим произведением нормализованных метрик.

Python-зависимости рекомендателя перечислены в файле `recommendation/requirements.txt`. В нём зафиксирована совместимая версия `scikit-learn`, необходимая для корректной загрузки и работы обученных ML-моделей. При первоначальной настройке все зависимости устанавливаются одной командой: `python -m pip install -r requirements.txt`.

При первом запуске создайте виртуальное окружение и установите зависимости:

```bash
cd recommendation
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
python app.py
```

В дальнейшем зависимости повторно устанавливать не нужно. В новом терминале достаточно активировать существующее окружение и запустить приложение:

```bash
cd recommendation
source .venv/bin/activate
python app.py
```

Повторная установка требуется только после изменения `requirements.txt` или удаления каталога `.venv`. После запуска откройте `http://127.0.0.1:8080`.

Тесты:

```bash
python3 -m unittest -v
```

## Реализации файловых систем

В проекте есть три реализации, все работают через общий интерфейс [`src/lib/filesystem.hpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/filesystem.hpp):

- `A` (`src/lib/A_fs`) - древовидная реализация `TreeFileSystem`.
- `B` (`src/lib/B_fs`) - отдельная реализация `FileSystemB`.
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
├── README.md
├── scripts/
│   └── run_bench_jobs.py
├── recommendation/
│   ├── static/
│   │   ├── index.html
│   │   ├── styles.css
│   │   └── app.js
│   ├── app.py
│   ├── model.py
│   ├── requirements.txt          # Python-зависимости рекомендателя
│   ├── main.py                   # Локальная точка входа / вспомогательный запуск
│   ├── model/                    # Сериализованные модели и scaler'ы по системам A/B/C
│       ├── A/
│       ├── B/
│       └── C/
│   └── test_model.py
├── ml_model/
│   ├── bench_results/            # CSV-результаты для обучения и анализа
│   ├── assets/                   # Графики и экспортированные изображения
│   ├── main.ipynb                # Общий ноутбук по моделям и данным
│   ├── systemA.ipynb             # Анализ и модель для системы A
│   ├── systemB.ipynb             # Анализ и модель для системы B
│   ├── systemC.ipynb             # Анализ и модель для системы C
│   ├── research.typ              # Typst-исходник ML-отчёта
│   └── research.pdf              # Собранный ML-отчёт
├── report/
│   ├── file_systems_description.typ
│   ├── report.typ
│   └── report.pdf
├── src/
│   ├── CMakeLists.txt
│   └── lib/
│       ├── A_fs/                  # Реализация A
│       ├── B_fs/                  # Реализация B
│       ├── C_fs/                  # Реализация C
│       ├── fs_generator/          # Генерация синтетической FS
│       ├── path_generator/        # Генерация путей для операций
│       ├── operation_generation/  # Генерация типов операций
│       ├── random/                # Вспомогательный RNG
│       └── benchmark/             # Бенчмарк и CLI runner
```

Ключевые зоны репозитория:

- `src/` содержит весь C++-код: реализации файловых систем, генераторы нагрузки и сам benchmark runner.
- `recommendation/` содержит готовое приложение, которое использует уже обученные модели для оценки систем A/B/C.
- `ml_model/` содержит исследовательские ноутбуки, исходные CSV-результаты, визуализации и отдельный ML-отчёт.
- `report/` содержит основной текстовый отчёт проекта в Typst и его PDF-сборку.

## Требования

Для сборки и запуска нужны:

- `CMake >= 3.16`
- компилятор с поддержкой `C++23`
- `Python 3`
- доступ в сеть при первой конфигурации `BUILD_TESTING=ON`, чтобы `FetchContent` подтянул `googletest`

Python-зависимостей для `scripts/run_bench_jobs.py` сейчас не требуется: скрипт использует только стандартную библиотеку.

Для работы с ноутбуками из `ml_model/` потребуется отдельное Python-окружение с `jupyter`, `pandas`, `scikit-learn`, `matplotlib` и связанными зависимостями анализа данных.

## Сборка

```bash
cmake -S src -B build
cmake --build build
```

После сборки основной исполняемый файл бенчмарка:

```bash
build/lib/benchmark/bench_run
```

Если генератор сборки у вас раскладывает бинарники иначе, ориентируйтесь на цель `bench_run` из [`src/lib/benchmark/CMakeLists.txt`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/CMakeLists.txt).

## Запуск одного benchmark job

CLI раннера реализован в [`src/lib/benchmark/bench_runner.cpp`](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/src/lib/benchmark/bench_runner.cpp).

Формат запуска:

```bash
build/lib/benchmark/bench_run FS Repeats Ops Profile OutputCsv
```

Где:

- `FS`: `A`, `B` или `C`
- `Repeats`: число повторов для каждой точки сетки
- `Ops`: число операций в одном прогоне
- `Profile`: профиль нагрузки
- `OutputCsv`: путь к CSV-файлу результата

Пример:

```bash
build/lib/benchmark/bench_run A 2 2000 build_system bench_results/results_A_build_system.csv
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
  --bench build/lib/benchmark/bench_run \
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

Скрипт также содержит грубую оценку ожидаемого времени выполнения job'ов.

## ML-артефакты

Директория `ml_model/` относится к исследовательской части проекта и не участвует в сборке C++-кода. Она нужна для анализа собранных benchmark-данных и подготовки моделей, которые потом используются в `recommendation/`.

В ней лежат:

- `bench_results/` с CSV-результатами прогонов для систем `A`, `B` и `C`;
- `systemA.ipynb`, `systemB.ipynb`, `systemC.ipynb` с разбором и обучением моделей по каждой системе;
- `main.ipynb` как общий ноутбук для сводного анализа;
- `assets/` с экспортированными графиками и изображениями;
- `research.typ` и `research.pdf` с отдельным ML-описанием результатов.

С практической точки зрения поток такой:

1. C++-бенчмарк генерирует CSV с измерениями.
2. Эти CSV используются в ноутбуках из `ml_model/` для анализа и подбора моделей.
3. Обученные артефакты экспортируются в `recommendation/model/` и используются веб-интерфейсом.

## Отчёт

Директория `report/` содержит текстовую часть проекта в Typst:

- [report/report.typ](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/report/report.typ) — основной исходник отчёта;
- [report/file_systems_description.typ](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/report/file_systems_description.typ) — описание реализаций файловых систем и связанных разделов;
- [report/report.pdf](/Users/igor/Documents/cpp_projects/prac/mega-cool-in-memory-file-systems-research/report/report.pdf) — собранная PDF-версия.

Если нужно править текст, логично считать `report/` независимым документным слоем, а `src/` и `ml_model/` — источниками технического содержания и результатов.

## Тесты

В проекте есть unit-тесты для:

- `A_fs`
- `B_fs`
- `C_fs`
- `fs_generator`
- `operation_generation`
- `path_generator`
- `random`

Если проект сконфигурирован с `BUILD_TESTING=ON`, `googletest` будет подтянут через `FetchContent`, после чего тесты можно запустить так:

```bash
ctest --test-dir build --output-on-failure
```

## Быстрый сценарий воспроизведения

Сборка:

```bash
cmake -S src -B build
cmake --build build
```

Один короткий прогон:

```bash
build/lib/benchmark/bench_run A 1 1000 db /tmp/results_A_db.csv
```

Небольшой пакетный прогон:

```bash
python3 scripts/run_bench_jobs.py \
  --bench build/lib/benchmark/bench_run \
  --fs A B C \
  --profiles db web build \
  --repeats 1 \
  --ops 1000 \
  --jobs 3 \
  --output-dir bench_results
```
