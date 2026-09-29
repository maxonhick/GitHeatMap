# git-heatmap

**Интерактивный анализатор активности Git-репозитория**

[![CI Build](https://github.com/maxonhick/GitHeatMap/actions/workflows/ci-build.yml/badge.svg)](https://github.com/maxonhick/GitHeatMap/actions/workflows/ci-build.yml)
[![Release](https://github.com/maxonhick/GitHeatMap/actions/workflows/release.yml/badge.svg)](https://github.com/maxonhick/GitHeatMap/actions/workflows/release.yml)
[![GitHub Release](https://img.shields.io/github/v/release/maxonhick/GitHeatMap?color=blue)](https://github.com/maxonhick/GitHeatMap/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## О проекте

`git-heatmap` — консольная утилита для быстрого анализа истории Git-репозитория. Она позволяет выявить «горячие зоны» кодовой базы:

- **Какие файлы менялись чаще всего** — подсчёт количества коммитов, затронувших каждый файл.
- **История последних изменений** — автор, сокращённый хэш коммита и дата последней правки.
- **Гибкая фильтрация** — срезы по веткам, расширениям, временным интервалам, маскам путей и авторам.
- **Экспорт результатов** — форматированная CLI-таблица, CSV, JSON и автономный HTML-отчёт с визуализацией активности.

---

## Статус реализации

> **Актуально для версии v0.1.5**

| Функция | Статус | Примечание |
|---------|--------|------------|
| **Ядро анализа** | ✅ Готово | Обход графа коммитов (`libgit2`), подсчёт изменений файлов через tree-diff |
| **CLI: Табличный вывод** | ✅ Готово | Форматированная консольная таблица с усечением топ-файлов (`--top`) |
| **CLI: Фильтры дат (`--since`, `--until`)** | ✅ Готово | Поддержка `YYYY-MM-DD`, ключевых слов (`today`, `yesterday`, `now`) и смещений (`2.weeks`, `two.weeks`, `3 days ago`) |
| **CLI: Фильтр автора (`-a, --author`)** | ✅ Готово | Подстрочный и wildcard-поиск (`*bot*`, `Alice`) без учёта регистра |
| **CLI: Фильтр ветки (`-b, --branch`)** | ✅ Готово | Анализ целевой ветки или ревизии (по умолчанию `HEAD`) |
| **CLI: Игнорирование мержей (`--no-merges`)** | ✅ Готово | Пропуск коммитов с несколькими родителями |
| **CLI: Расширения файлов (`-e, --extensions`)** | ✅ Готово | Фильтрация по списку расширений (`.cpp,.h,.md`) |
| **CLI: Маски исключения (`--exclude-patterns`)** | ✅ Готово | Исключение путей и папок по шаблонам (поддержка `*` и `?`) |
| **CLI: Сортировка (`--sort`)** | ✅ Готово | По количеству коммитов (`count`), дате изменения (`lastchange`) или имени (`file`) |
| **Вывод: CSV / JSON** | ✅ Готово | Экспорт для электронных таблиц или машинной обработки |
| **Вывод: HTML (`--format html`)** | ✅ Готово | Автономный одностраничный отчёт с CSS-полосками активности |
| **Тестирование (GoogleTest + CTest)** | ✅ Готово | Модульные тесты фильтров/принтера и интеграционные тесты с временным репозиторием |
| **CI/CD** | ✅ Готово | Кроссплатформенная сборка и прогон тестов (Linux, macOS, Windows) |
| **График активности (`--activity`)** | ❌ Запланировано | Распределение коммитов по часам и дням недели |
| **TUI-режим (`--tui`)** | ❌ Запланировано | Интерактивный терминальный интерфейс на базе FTXUI |

---
## Установка

### Готовые бинарные файлы (Releases)

Вы можете скачать уже скомпилированную утилиту для вашей ОС на странице [**Releases**](https://github.com/maxonhick/GitHeatMap/releases/latest):

| Платформа | Архитектура | Файл архива |
| :--- | :--- | :--- |
| **Linux** | x86_64 | `git-heatmap-linux-x86_64.tar.gz` |
| **macOS** | Universal / Apple Silicon / Intel | `git-heatmap-macos-universal.tar.gz` |
| **Windows** | x64 | `git-heatmap-windows-x86_64.tar.gz` |

#### Быстрый старт на Linux / macOS:
```bash
# Распаковать архив
tar -xzf git-heatmap-*.tar.gz

# (Опционально) Переместить в системный путь
sudo mv GitHeatMap /usr/local/bin/git-heatmap

# Проверка
git-heatmap --help
```

---
### Сборка и установка

#### Зависимости
- C++17 компилятор (Clang, GCC или MSVC)
- CMake 3.10+
- `libgit2`
- Ninja (рекомендуется)

**macOS:**
```bash
brew install libgit2 pkg-config ninja

```

**Linux (Ubuntu/Debian):**

```bash
sudo apt-get update && sudo apt-get install -y libgit2-dev pkg-config ninja-build

```

**Windows (vcpkg):**

```powershell
vcpkg install libgit2:x64-windows

```

#### Сборка проекта

```bash
# Конфигурация
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Сборка утилиты и тестов
cmake --build build --config Release

```

#### Запуск тестов

```bash
ctest --test-dir build --output-on-failure
# Или напрямую:
./build/githeatmap_tests

```

---

## Примеры использования

### Базовый анализ

```bash
# Анализ текущего репозитория (вывод топ-10 файлов)
./build/GitHeatMap .

# Вывод топ-20 файлов с сортировкой по времени последнего изменения
./build/GitHeatMap . -n 20 --sort lastchange

```

### Фильтрация по датам

```bash
# Анализ за последние 2 недели
./build/GitHeatMap . --since 2.weeks

# Временной интервал с естественным языком
./build/GitHeatMap . --since "a month ago" --until yesterday

# Анализ коммитов, сделанных строго сегодня
./build/GitHeatMap . --since today

```

### Фильтры файлов, авторов и веток

```bash
# Только исходники C++ и заголовки, исключая тесты и сборку
./build/GitHeatMap . -e .cpp,.hpp --exclude-patterns "build/*,tests/*"

# Анализ конкретной ветки и коммитов автора "Ivan" без merge-коммитов
./build/GitHeatMap . -b develop -a "Ivan*" --no-merges

```

### Экспорт отчётов

```bash
# Сохранение отчёта в CSV
./build/GitHeatMap . --format csv --output report.csv

# Сохранение в JSON
./build/GitHeatMap . --format json --output stats.json

# Генерация интерактивного HTML-отчёта с графиками активности
./build/GitHeatMap . --format html --output heatmap.html -n 50

```

---

## Лицензия

Проект распространяется под лицензией [MIT](https://www.google.com/search?q=LICENSE).