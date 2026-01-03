cmake_minimum_required(VERSION 3.16)
project(LocalTaskManager LANGUAGES CXX)

# Общие настройки C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========================
#  ODB: генерация кода
# ========================

# Модели (сюда добавляешь новые заголовки с сущностями)
set(MODEL_HEADERS
  ${PROJECT_SOURCE_DIR}/include/models/task.hxx
)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(ODB_COMPILER odb)

# Важно: --std c++11 относится к коду МОДЕЛЕЙ.
# Не используй в моделях фичи новее C++11, или поменяй этот флаг.
set(ODB_FLAGS
  -d sqlite
  --std c++11
  --generate-query
  --generate-schema
  -I${PROJECT_SOURCE_DIR}/include
)

set(ODB_GENERATED_SOURCES)
set(ODB_GENERATED_HEADERS)

foreach(h ${MODEL_HEADERS})
  get_filename_component(name ${h} NAME_WE)
  set(src ${GENERATED_DIR}/${name}-odb.cxx)
  set(hxx ${GENERATED_DIR}/${name}-odb.hxx)

  list(APPEND ODB_GENERATED_SOURCES ${src})
  list(APPEND ODB_GENERATED_HEADERS ${hxx})

  add_custom_command(
    OUTPUT ${src} ${hxx}
    COMMAND ${ODB_COMPILER} ${ODB_FLAGS} -o ${GENERATED_DIR} ${h}
    DEPENDS ${h}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Генерация ODB-кода для ${h}"
    VERBATIM
  )
endforeach()

# ========================
#  Исполняемый файл
# ========================

add_executable(task_manager
  src/main.cpp
  ${ODB_GENERATED_SOURCES}
)

target_include_directories(task_manager
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/models
    ${GENERATED_DIR}
)

# ========================
#  Линковка с библиотеками
# ========================

find_library(ODB_LIB        NAMES odb)
find_library(ODB_SQLITE_LIB NAMES odb-sqlite)
find_library(SQLITE3_LIB    NAMES sqlite3)

if(NOT ODB_LIB OR NOT ODB_SQLITE_LIB OR NOT SQLITE3_LIB)
  message(FATAL_ERROR "Не найдены необходимые библиотеки ODB/SQLite")
endif()

target_link_libraries(task_manager
  PRIVATE
    ${ODB_LIB}
    ${ODB_SQLITE_LIB}
    ${SQLITE3_LIB}
)