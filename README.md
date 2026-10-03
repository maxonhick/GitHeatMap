# git-heatmap

**Интерактивный анализатор активности Git-репозитория**

[![CI Build](https://github.com/maxonhick/GitHeatMap/actions/workflows/ci-build.yml/badge.svg)](https://github.com/maxonhick/GitHeatMap/actions/workflows/ci-build.yml)
[![Release](https://github.com/maxonhick/GitHeatMap/actions/workflows/release.yml/badge.svg)](https://github.com/maxonhick/GitHeatMap/actions/workflows/release.yml)
[![GitHub Release](https://img.shields.io/github/v/release/maxonhick/GitHeatMap?color=blue)](https://github.com/maxonhick/GitHeatMap/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## О проекте

`git-heatmap` — кроссплатформенная консольная утилита для детального анализа истории изменений Git-репозитория. Она позволяет выявить «горячие зоны» кодовой базы и распределение рабочей нагрузки во времени:

- **Какие файлы менялись чаще всего** — подсчёт количества коммитов, затронувших каждый файл кодовой базы.
- **История последних изменений** — автор, сокращённый хэш и точная дата последней модификации.
- **Распределение активности во времени** — разбивка коммитов по часам дня, дням недели, дням месяца и месяцам года.
- **Гибкая фильтрация** — срезы по веткам, расширениям, временным интервалам, маскам путей и авторам.
- **Мультиформатный экспорт** — форматированная CLI-таблица с ASCII-гистограммами, CSV (включая отдельный файл активности), структурированный JSON и автономный HTML-отчёт с адаптивным виджетом активности.

---

## Статус реализации

> **Актуально для версии v1.0.0**

| Функция | Статус | Примечание |
|---------|--------|------------|
| **Ядро анализа** | ✅ Готово | Обход графа коммитов (`libgit2`), сбор статистики файлов через tree-diff |
| **CLI: Табличный вывод** | ✅ Готово | Консольная таблица с ограничением топ-файлов (`--top`) |
| **CLI: Фильтры дат (`--since`, `--until`)** | ✅ Готово | Поддержка `YYYY-MM-DD`, ключевых слов (`today`, `yesterday`, `now`) и смещений (`2.weeks`, `two.weeks`, `3 days ago`) |
| **CLI: Фильтр автора (`-a, --author`)** | ✅ Готово | Подстрочный и wildcard-поиск (`*bot*`, `Alice`) без учёта регистра |
| **CLI: Фильтр ветки (`-b, --branch`)** | ✅ Готово | Анализ целевой ветки или ревизии (по умолчанию `HEAD`) |
| **CLI: Игнорирование мержей (`--no-merges`)** | ✅ Готово | Пропуск merge-коммитов с несколькими родителями |
| **CLI: Расширения файлов (`-e, --extensions`)** | ✅ Готово | Фильтрация по списку расширений (`.cpp,.h,.md`) |
| **CLI: Маски исключения (`--exclude-patterns`)** | ✅ Готово | Исключение файлов и папок по шаблонам (поддержка `*` и `?`) |
| **CLI: Сортировка (`--sort`)** | ✅ Готово | По количеству коммитов (`count`), дате изменения (`lastchange`) или имени (`file`) |
| **Мониторинг активности (`--activity`)** | ✅ Готово | Гистограммы коммитов: `hour` (0–23), `wday` (Пн–Вс), `mday` (1–31), `month` (Янв–Дек) |
| **Вывод: CSV / JSON** | ✅ Готово | Сохранение структуры файлов и упорядоченных срезов активности |
| **Вывод: HTML (`--format html`)** | ✅ Готово | Автономный отчёт с CSS-виджетом активности (Activity Bar) и тултипами |
| **Тестирование (GoogleTest + CTest)** | ✅ Готово | Модульные и интеграционные тесты анализатора, принтера и фильтров |
| **CI/CD и статические релизы** | ✅ Готово | Кроссплатформенная сборка автономных бинарников без внешних динамических библиотек |
| **TUI-режим (`--tui`)** | ❌ Запланировано | Интерактивный терминальный интерфейс на базе FTXUI |

---
## Установка

### Готовые бинарные файлы (Releases)

Автономные исполняемые файлы для всех систем доступны на странице [**GitHub Releases**](https://github.com/maxonhick/GitHeatMap/releases/latest):

| Платформа   | Архитектура                       | Файл архива                          |
| :---------- | :-------------------------------- | :----------------------------------- |
| **Linux**   | x86_64                            | `git-heatmap-linux-x86_64.tar.gz`    |
| **macOS**   | Universal (Apple Silicon & Intel) | `git-heatmap-macos-universal.tar.gz` |
| **Windows** | x64                               | `git-heatmap-windows-x86_64.tar.gz`  

#### Быстрый запуск на Linux / macOS:
```bash
# Распаковать архив
tar -xzf git-heatmap-*.tar.gz

# (Опционально) Переместить в системный путь
sudo mv GitHeatMap /usr/local/bin/git-heatmap

# Проверка работоспособности
git-heatmap --help
````
### Сборка из исходников

Все необходимые зависимости (включая `libgit2` и `GoogleTest`) автоматически загружаются и собираются статически через CMake `FetchContent`.
#### Требования:

- C++17 совместимый компилятор (Clang, GCC или MSVC)
- CMake 3.14+
- Ninja (рекомендуется)
```Bash
# Конфигурация проекта с включенными тестами
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Сборка приложения и тестового набора
cmake --build build --config Release

# Запуск тестов
ctest --test-dir build --output-on-failure
```
## Примеры использования

### Базовый анализ
```bash
# Анализ текущего репозитория (вывод топ-10 файлов)
./build/GitHeatMap .

# Вывод топ-20 файлов с сортировкой по времени последнего изменения
./build/GitHeatMap . -n 20 --sort lastchange
```
### Мониторинг активности коммитов (`--activity`)

```bash
# Распределение активности по дням недели (Пн-Вс)
./build/GitHeatMap . --activity wday

# Активность по часам суток (00:00 - 23:00) за последние 2 недели
./build/GitHeatMap . --activity hour --since 2.weeks

# Активность автора по дням месяца
./build/GitHeatMap . --activity mday -a "John Doe"

# Сезонная активность по месяцам года
./build/GitHeatMap . --activity month
```
### Фильтрация по датам

```bash
# Смещения с использованием точек и цифр
./build/GitHeatMap . --since 2.weeks

# С использованием естественного языка
./build/GitHeatMap . --since "a month ago" --until yesterday

# Анализ коммитов строго за сегодняшний день
./build/GitHeatMap . --since today
```
### Фильтры файлов, авторов и веток

```bash
# Только C++ исходники и заголовки, исключая папки build и tests
./build/GitHeatMap . -e .cpp,.hpp --exclude-patterns "build/*,tests/*"

# Анализ конкретной ветки разработчика без merge-коммитов
./build/GitHeatMap . -b feature/login -a "Alice*" --no-merges
```
### Мультиформатный экспорт

```bash
# Экспорт таблицы файлов в CSV (при ключе --activity создастся дополнительный report.csv.activity.csv)
./build/GitHeatMap . --activity wday --format csv --output report.csv

# Сохранение полной структуры (активность + файлы) в отсортированный JSON
./build/GitHeatMap . --activity hour --format json --output stats.json

# Генерация автономного HTML-отчёта с графиком активности и таблицей
./build/GitHeatMap . --activity wday --format html --output heatmap.html -n 50
```
## Лицензия

Проект распространяется под лицензией [MIT](https://www.google.com/search?q=LICENSE).