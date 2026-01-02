# LocalTaskManager (C++ + ODB + SQLite)

Минимальный, но расширяемый каркас приложения менеджера задач на C++ с использованием
ORM ODB и SQLite. Сборка и разработка ведутся внутри Docker-контейнера на базе Ubuntu.

## Структура проекта

```text
LocalTaskManager/
  Dockerfile
  CMakeLists.txt
  README.md

  include/
    models/
      task.hxx          # модель Task

  src/
    main.cxx            # точка входа

  data/                 # БД (tasks.db), создаётся при запуске
  build/                # каталог сборки CMake
```

## Требования

- Docker (или Podman, который эмулирует Docker CLI)
- Git (опционально, если клонируете репозиторий)

## Сборка Docker-образа

В корне проекта:

```bash
docker build -t local-task-manager-dev .
```

(Если используете Podman, команда та же — он подменяет `docker`.)

## Запуск dev-контейнера

В корне проекта:

```bash
docker run -it --rm \
  -v "$PWD":/app \
  -w /app \
  local-task-manager-dev \
  bash
```

- `-v "$PWD":/app` — монтирует ваш код в контейнер;
- `-w /app` — рабочая директория внутри контейнера.

Все последующие команды выполняются **внутри** контейнера.

## Сборка проекта (CMake)

1. Конфигурация:

   ```bash
   cmake -S . -B build
   ```

   (Можно добавить `-G Ninja`, если хотите использовать Ninja.)

2. Сборка:

   ```bash
   cmake --build build
   ```

   В процессе:
   - CMake запустит `odb` для `include/models/task.hxx`;
   - ODB сгенерирует файлы `task-odb.cxx`/`task-odb.hxx` в `build/generated`;
   - затем соберётся исполняемый файл `task_manager`.

## Запуск приложения

После успешной сборки:

```bash
./build/task_manager
```

Первый запуск:

- создаст директорию `data/`, если её ещё нет;
- создаст файл БД `data/tasks.db`;
- создаст схему БД (таблицу для задач);
- добавит две тестовые задачи и выведет их в stdout.

Повторные запуски:

- если `data/tasks.db` уже существует, схема не пересоздаётся;
- новые тестовые задачи будут добавляться при каждом запуске.

Пример вывода:

```text
Схема БД создана в data/tasks.db
Созданы задачи с id: 1, 2
Незавершённые задачи:
1: Выучить ODB — Написать первую программу [completed=no]
2: Купить молоко — 2 литра [completed=no]
```

## Просмотр БД

Внутри контейнера:

```bash
sqlite3 data/tasks.db
```

Например:

```sql
.tables
.schema task
SELECT * FROM task;
```

## Как масштабировать (добавлять новые сущности)

1. **Создайте новую модель** в `include/models`, например `project.hxx`:

   ```cpp
   #pragma db object
   class project
   {
     // ...
   };
   ```

   Не забудьте:
   - `#include <odb/core.hxx>`
   - `friend class odb::access;`
   - поле `id` с `#pragma db id auto` (если нужен автоинкрементный ключ).

2. **Добавьте заголовок модели в `CMakeLists.txt`** в список `MODEL_HEADERS`:

   ```cmake
   set(MODEL_HEADERS
     ${PROJECT_SOURCE_DIR}/include/models/task.hxx
     ${PROJECT_SOURCE_DIR}/include/models/project.hxx
   )
   ```

3. **Пересоберите проект**:

   ```bash
   cmake -S . -B build
   cmake --build build
   ```

   CMake вызовет `odb` для новой модели, сгенерирует `project-odb.cxx/.hxx`,
   и вы сможете использовать её в коде (подключая `#include "project-odb.hxx"`).

4. **Используйте новую модель в коде** (например, в `src/main.cxx` или в
   новых модулях):

   ```cpp
   #include "models/project.hxx"
   #include "project-odb.hxx"
   ```

   и работайте через ODB (`db.persist`, `db.load`, `db.query`, и т.д.).
