#include <iostream>
#include <filesystem>

#include <sqlite3.h>

#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/query.hxx>
#include <odb/result.hxx>

#include "models/task.hxx"
#include "task-odb.hxx"

namespace fs = std::filesystem;

int main()
{
  try
  {
    const std::string db_dir  = "data";
    const std::string db_file = db_dir + "/tasks.db";

    // Создаём директорию под БД, если её нет
    fs::create_directories(db_dir);
    bool new_db = !fs::exists(db_file);

    // Открываем/создаём БД SQLite
    odb::sqlite::database db(
        db_file,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    );

    // Если БД новая — создаём схему (таблицы)
    if (new_db)
    {
      odb::transaction t(db.begin());
      odb::schema_catalog::create_schema(db);
      t.commit();
      std::cout << "Схема БД создана в " << db_file << std::endl;
    }

    // Добавляем несколько задач
    {
      odb::transaction t(db.begin());

      task t1("Выучить ODB", "Написать первую программу");
      task t2("Купить молоко", "2 литра");

      db.persist(t1);
      db.persist(t2);

      t.commit();

      std::cout << "Созданы задачи с id: "
                << t1.id() << ", " << t2.id() << '\n';
    }

    // Выводим все незавершённые задачи
    {
      odb::transaction t(db.begin());

      using query  = odb::query<task>;
      using result = odb::result<task>;

      result r(db.query<task>(query::completed == false));

      std::cout << "Незавершённые задачи:\n";
      for (const task& tk : r)
      {
        std::cout << tk.id() << ": " << tk.title()
                  << " — " << tk.description()
                  << " [completed=" << (tk.completed() ? "yes" : "no") << "]\n";
      }

      t.commit();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Ошибка: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}