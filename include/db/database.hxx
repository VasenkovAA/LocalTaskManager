#pragma once

#include <memory>
#include <QString>

namespace odb { namespace sqlite { class database; } }

namespace db
{
  using DbPtr = std::shared_ptr<odb::sqlite::database>;

  struct OpenResult
  {
    DbPtr db;
    bool createdNew = false;
  };

  OpenResult openDatabaseReadOrCreate(const QString& dbPath);

  void ensureSchemaBestEffort(odb::sqlite::database& db);
  void seedIfEmpty(odb::sqlite::database& db);
}