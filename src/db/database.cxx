#include "db/database.hxx"

#include <QDir>
#include <QFileInfo>
#include <stdexcept>
#include <string>

#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>
#include <sqlite3.h>

#include "models/kanban_board.hxx"
#include "models/kanban_column.hxx"
#include "models/task.hxx"
#include "models/task_category.hxx"

#include "kanban_board-odb.hxx"
#include "kanban_column-odb.hxx"
#include "task-odb.hxx"
#include "task_category-odb.hxx"
namespace db {
static void ensureParentDirExists(const QString &filePath) {
  QFileInfo fi(filePath);
  QDir dir(fi.absolutePath());
  if (!dir.exists() && !dir.mkpath("."))
    throw std::runtime_error(
        ("Failed to create directory: " + dir.absolutePath()).toStdString());
}

OpenResult openDatabaseReadOrCreate(const QString &dbPath) {
  if (dbPath.trimmed().isEmpty())
    throw std::runtime_error("DB path is empty");

  const QString clean = QDir::cleanPath(dbPath);
  QFileInfo fi(clean);

  if (fi.exists() && fi.isDir())
    throw std::runtime_error(
        ("DB path points to a directory: " + clean).toStdString());

  const bool exists = fi.exists();

  if (exists && fi.size() == 0)
    throw std::runtime_error(
        ("Database file exists but is empty (0 bytes): " + clean)
            .toStdString());

  const bool treatAsNew = !exists;

  if (treatAsNew)
    ensureParentDirExists(clean);

  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  if (treatAsNew)
    flags |= SQLITE_OPEN_CREATE;

  const std::string pathUtf8 = clean.toUtf8().toStdString();

  OpenResult out;
  out.db = std::make_shared<odb::sqlite::database>(pathUtf8, flags);
  out.createdNew = treatAsNew;
  return out;
}

void ensureSchemaBestEffort(odb::sqlite::database &db) {
  odb::transaction t(db.begin());
  odb::schema_catalog::create_schema(db);
  t.commit();
}

void seedIfEmpty(odb::sqlite::database &db) {
  odb::transaction t(db.begin());

  bool hasBoard = false;
  for (const auto &b : db.query<KanbanBoard>()) {
    (void)b;
    hasBoard = true;
    break;
  }

  if (hasBoard) {
    t.commit();
    return;
  }

  KanbanBoard board("My Board");
  db.persist(board);

  KanbanColumn todo(board.id(), "Todo", 0);
  KanbanColumn doing(board.id(), "Doing", 1);
  KanbanColumn done(board.id(), "Done", 2);
  db.persist(todo);
  db.persist(doing);
  db.persist(done);

  // Категории (если хочешь seed)
  TaskCategory bug(board.id(), "Bug", 0, "#e74c3c");
  TaskCategory feature(board.id(), "Feature", 1, "#3498db");
  db.persist(bug);
  db.persist(feature);

  Task a(board.id(), todo.id(), "First task", "Seed", 0);
  a.category_id(bug.id()); // опционально
  Task b(board.id(), doing.id(), "Second task", "", 0);
  Task c(board.id(), done.id(), "Third task", "", 0);

  db.persist(a);
  db.persist(b);
  db.persist(c);

  t.commit();
}
} // namespace db