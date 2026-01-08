cmake_minimum_required(VERSION 3.16)
project(LocalTaskManager LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# ========================
#  ODB: генерация кода
# ========================
set(MODEL_HEADERS
  ${PROJECT_SOURCE_DIR}/include/models/kanban_board.hxx
  ${PROJECT_SOURCE_DIR}/include/models/kanban_column.hxx
  ${PROJECT_SOURCE_DIR}/include/models/task.hxx
  ${PROJECT_SOURCE_DIR}/include/models/task_category.hxx
)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

find_program(ODB_COMPILER NAMES odb REQUIRED)

set(ODB_FLAGS
  -d sqlite
  --std c++11
  --generate-query
  --generate-schema
  -I${PROJECT_SOURCE_DIR}/include
  -I${PROJECT_SOURCE_DIR}/include/models
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
    COMMAND ${ODB_COMPILER} ${ODB_FLAGS} --output-dir ${GENERATED_DIR} ${h}
    DEPENDS ${h}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Генерация ODB-кода для ${h}"
    VERBATIM
  )
endforeach()

set_source_files_properties(${ODB_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)
set_source_files_properties(${ODB_GENERATED_HEADERS} PROPERTIES GENERATED TRUE)

# ========================
#  Зависимости ODB/SQLite
# ========================
find_library(ODB_LIB        NAMES odb)
find_library(ODB_SQLITE_LIB NAMES odb-sqlite)
find_library(SQLITE3_LIB    NAMES sqlite3)

if(NOT ODB_LIB OR NOT ODB_SQLITE_LIB OR NOT SQLITE3_LIB)
  message(FATAL_ERROR "Не найдены необходимые библиотеки ODB/SQLite")
endif()

# ========================
#  Общая библиотека (core)
# ========================
add_library(task_manager_core
  src/app/settings.cxx
  src/db/database.cxx
  src/db/odb_kanban_api.cxx
  ${ODB_GENERATED_SOURCES}

  include/app/settings.hxx
  include/db/database.hxx
  include/core/kanban_api.hxx
  include/db/odb_kanban_api.hxx
)

target_include_directories(task_manager_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/models
    ${GENERATED_DIR}
)

target_link_libraries(task_manager_core
  PUBLIC
    Qt5::Core
    ${ODB_LIB}
    ${ODB_SQLITE_LIB}
    ${SQLITE3_LIB}
)

# ========================
#  GUI app
# ========================
add_executable(task_manager
  src/main.cpp

  src/gui/column_list_widget.cxx
  src/gui/main_window.cxx
  src/gui/kanban_column_widget.cxx
  src/gui/task_edit_dialog.cxx

  include/gui/column_list_widget.hxx
  include/gui/main_window.hxx
  include/gui/kanban_column_widget.hxx
  include/gui/task_edit_dialog.hxx
)

target_link_libraries(task_manager
  PRIVATE
    task_manager_core
    Qt5::Widgets
)

