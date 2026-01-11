#pragma once

#include <QString>
#include <memory>

namespace odb {
namespace sqlite {
class database;
}
} // namespace odb

namespace db {
using DbPtr = std::shared_ptr<odb::sqlite::database>;

struct OpenResult {
  DbPtr db;
  bool createdNew = false;
};

OpenResult openDatabaseReadOrCreate(const QString &dbPath);

void ensureSchemaBestEffort(odb::sqlite::database &db);
void seedIfEmpty(odb::sqlite::database &db);
} // namespace db