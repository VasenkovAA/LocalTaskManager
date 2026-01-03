#include <iostream>
#include <filesystem>


#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/query.hxx>
#include <odb/result.hxx>

#include "models/task.hxx"
#include "task-odb.hxx"


int main(){
  try{
    const std::string DB_DIR = "data";
    const std::string DB_NAME = "task_database.db";
    const std::string DB_PATH = DB_DIR + "/" + DB_NAME;

    std::filesystem::create_directories(DB_DIR);
    bool new_db = !std::filesystem::exists(DB_PATH);
    odb::sqlite::database db(DB_PATH, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    {
      odb::transaction t(db.begin());
      odb::schema_catalog::create_schema(db);
      t.commit();
    }
    {
      odb::transaction t(db.begin());
      Task task_1("test1", "test1_1");
      Task task_2("test2", "test2_1");
      db.persist(task_1);
      db.persist(task_2);
      t.commit();
    }
    {
      odb::transaction t(db.begin());
      using query = odb::query<Task>;
      using result = odb::result<Task>;
      result r(db.query<Task>());
      for (const Task& it : r){
        std::cout<<it.title()<<" : "<<it.description()<<std::endl;
      }
      t.commit();
    }

  }catch (const std::exception& e){
    std::cerr << "ERROR: "<< e.what() << std::endl;
    return 1;
  }
  return 0;
};
