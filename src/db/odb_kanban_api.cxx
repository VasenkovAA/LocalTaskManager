#include "db/odb_kanban_api.hxx"

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>

#include "models/kanban_board.hxx"
#include "models/kanban_column.hxx"
#include "models/task.hxx"
#include "models/task_category.hxx"

#include "kanban_board-odb.hxx"
#include "kanban_column-odb.hxx"
#include "task-odb.hxx"
#include "task_category-odb.hxx"

namespace {
unsigned long to_ul(core::Id id) {
  if (id > static_cast<core::Id>(std::numeric_limits<unsigned long>::max()))
    throw std::runtime_error(
        "Id does not fit into unsigned long (platform mismatch)");
  return static_cast<unsigned long>(id);
}

core::Id to_id(unsigned long id) { return static_cast<core::Id>(id); }

core::BoardDto to_dto(const KanbanBoard &b) {
  core::BoardDto d;
  d.id = to_id(b.id());
  d.name = b.name();
  d.description = b.description();
  return d;
}

core::ColumnDto to_dto(const KanbanColumn &c) {
  core::ColumnDto d;
  d.id = to_id(c.id());
  d.boardId = to_id(c.board_id());
  d.name = c.name();
  d.sortOrder = c.sort_order();
  return d;
}

core::TaskDto to_dto(const Task &t) {
  core::TaskDto d;
  d.id = to_id(t.id());
  d.boardId = to_id(t.board_id());
  d.columnId = to_id(t.column_id());
  d.title = t.title();
  d.description = t.description();
  d.sortOrder = t.sort_order();
  d.archived = t.archived();

  auto cat = t.category_id();
  if (!cat.null())
    d.categoryId = to_id(cat.get());

  return d;
}
core::CategoryDto to_dto(const TaskCategory &c) {
  core::CategoryDto d;
  d.id = to_id(c.id());
  d.boardId = to_id(c.board_id());
  d.name = c.name();
  d.sortOrder = c.sort_order();
  d.color = c.color();
  return d;
}
} // namespace

namespace db {
std::vector<core::CategoryDto> OdbKanbanApi::listCategories(core::Id boardId) {
  const unsigned long bid = to_ul(boardId);
  odb::transaction t(db_->begin());

  using CQ = odb::query<TaskCategory>;
  auto res = db_->query<TaskCategory>(CQ::board_id == bid);

  std::vector<core::CategoryDto> out;
  for (const auto &c : res)
    out.push_back(to_dto(c));

  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.sortOrder != b.sortOrder)
      return a.sortOrder < b.sortOrder;
    return a.id < b.id;
  });

  t.commit();
  return out;
}

core::Id OdbKanbanApi::createCategory(core::Id boardId, const std::string &name,
                                      const std::string &color) {
  if (name.empty())
    throw std::runtime_error("createCategory: name is empty");

  const unsigned long bid = to_ul(boardId);
  odb::transaction t(db_->begin());

  int maxOrder = -1;
  using CQ = odb::query<TaskCategory>;
  for (const auto &c : db_->query<TaskCategory>(CQ::board_id == bid))
    maxOrder = std::max(maxOrder, c.sort_order());

  TaskCategory cat(bid, name, maxOrder + 1, color);
  db_->persist(cat);

  t.commit();
  return to_id(cat.id());
}

void OdbKanbanApi::deleteCategory(core::Id categoryId) {
  const unsigned long cid = to_ul(categoryId);
  odb::transaction t(db_->begin());

  // очистить category_id у задач
  using TQ = odb::query<Task>;
  for (const auto &taskRow : db_->query<Task>(TQ::category_id == cid)) {
    Task tmp = taskRow;
    tmp.clear_category();
    db_->update(tmp);
  }

  db_->erase<TaskCategory>(cid);
  t.commit();
}

void OdbKanbanApi::setTaskCategory(core::Id taskId,
                                   std::optional<core::Id> categoryId) {
  const unsigned long tid = to_ul(taskId);
  odb::transaction t(db_->begin());

  std::unique_ptr<Task> task(db_->load<Task>(tid));
  if (!task)
    throw std::runtime_error("setTaskCategory: task not found");

  if (!categoryId.has_value()) {
    task->clear_category();
    db_->update(*task);
    t.commit();
    return;
  }

  const unsigned long cid = to_ul(*categoryId);

  std::unique_ptr<TaskCategory> cat(db_->load<TaskCategory>(cid));
  if (!cat)
    throw std::runtime_error("setTaskCategory: category not found");

  if (cat->board_id() != task->board_id())
    throw std::runtime_error(
        "setTaskCategory: category belongs to different board");

  task->category_id(cid);
  db_->update(*task);

  t.commit();
}

OdbKanbanApi::OdbKanbanApi(std::shared_ptr<odb::sqlite::database> db)
    : db_(std::move(db)) {
  if (!db_)
    throw std::runtime_error("OdbKanbanApi: db is null");
}

std::vector<core::BoardDto> OdbKanbanApi::listBoards() {
  odb::transaction t(db_->begin());

  std::vector<core::BoardDto> out;
  for (const auto &b : db_->query<KanbanBoard>())
    out.push_back(to_dto(b));

  std::sort(out.begin(), out.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  t.commit();
  return out;
}

core::Id OdbKanbanApi::createBoard(const std::string &name,
                                   const std::string &description) {
  if (name.empty())
    throw std::runtime_error("createBoard: name is empty");

  odb::transaction t(db_->begin());
  KanbanBoard b(name, description);
  db_->persist(b);
  t.commit();
  return to_id(b.id());
}

void OdbKanbanApi::deleteBoard(core::Id boardId) {
  const unsigned long bid = to_ul(boardId);

  odb::transaction t(db_->begin());

  using TQ = odb::query<Task>;
  for (const auto &task : db_->query<Task>(TQ::board_id == bid))
    db_->erase<Task>(task.id());

  using CQ = odb::query<KanbanColumn>;
  for (const auto &col : db_->query<KanbanColumn>(CQ::board_id == bid))
    db_->erase<KanbanColumn>(col.id());

  using CatQ = odb::query<TaskCategory>;
  for (const auto &c : db_->query<TaskCategory>(CatQ::board_id == bid))
    db_->erase<TaskCategory>(c.id());

  db_->erase<KanbanBoard>(bid);
  t.commit();
}

std::vector<core::ColumnDto> OdbKanbanApi::listColumns(core::Id boardId) {
  const unsigned long bid = to_ul(boardId);

  odb::transaction t(db_->begin());

  using CQ = odb::query<KanbanColumn>;
  auto res = db_->query<KanbanColumn>(CQ::board_id == bid);

  std::vector<core::ColumnDto> out;
  for (const auto &c : res)
    out.push_back(to_dto(c));

  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.sortOrder != b.sortOrder)
      return a.sortOrder < b.sortOrder;
    return a.id < b.id;
  });

  t.commit();
  return out;
}

core::Id OdbKanbanApi::createColumn(core::Id boardId, const std::string &name) {
  if (name.empty())
    throw std::runtime_error("createColumn: name is empty");

  const unsigned long bid = to_ul(boardId);

  odb::transaction t(db_->begin());

  int maxOrder = -1;
  using CQ = odb::query<KanbanColumn>;
  for (const auto &c : db_->query<KanbanColumn>(CQ::board_id == bid))
    maxOrder = std::max(maxOrder, c.sort_order());

  KanbanColumn col(bid, name, maxOrder + 1);
  db_->persist(col);

  t.commit();
  return to_id(col.id());
}

void OdbKanbanApi::deleteColumn(core::Id columnId) {
  const unsigned long cid = to_ul(columnId);

  odb::transaction t(db_->begin());

  // Нужно знать board_id, чтобы перенумеровать остальные колонки
  std::unique_ptr<KanbanColumn> colPtr;
  try {
    colPtr.reset(db_->load<KanbanColumn>(cid));
  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::string("deleteColumn: failed to load column: ") + e.what());
  }

  if (!colPtr)
    throw std::runtime_error("deleteColumn: column not found");

  const unsigned long bid = colPtr->board_id();

  using TQ = odb::query<Task>;
  for (const auto &task : db_->query<Task>(TQ::column_id == cid))
    db_->erase<Task>(task.id());

  db_->erase<KanbanColumn>(cid);

  // renumber remaining columns in this board
  using CQ = odb::query<KanbanColumn>;
  std::vector<KanbanColumn> cols;
  for (const auto &c : db_->query<KanbanColumn>(CQ::board_id == bid))
    cols.push_back(c);

  std::sort(cols.begin(), cols.end(),
            [](const KanbanColumn &a, const KanbanColumn &b) {
              if (a.sort_order() != b.sort_order())
                return a.sort_order() < b.sort_order();
              return a.id() < b.id();
            });

  for (int i = 0; i < static_cast<int>(cols.size()); ++i) {
    auto c = cols[i];
    c.sort_order(i);
    db_->update(c);
  }

  t.commit();
}

std::vector<core::TaskDto>
OdbKanbanApi::listTasksForBoard(core::Id boardId, bool includeArchived) {
  const unsigned long bid = to_ul(boardId);

  odb::transaction t(db_->begin());

  using TQ = odb::query<Task>;
  auto q = (TQ::board_id == bid);
  if (!includeArchived)
    q = q && (TQ::archived == false);

  std::vector<core::TaskDto> out;
  for (const auto &task : db_->query<Task>(q))
    out.push_back(to_dto(task));

  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.columnId != b.columnId)
      return a.columnId < b.columnId;
    if (a.sortOrder != b.sortOrder)
      return a.sortOrder < b.sortOrder;
    return a.id < b.id;
  });

  t.commit();
  return out;
}

std::vector<core::TaskDto>
OdbKanbanApi::listTasksForColumn(core::Id columnId, bool includeArchived) {
  const unsigned long cid = to_ul(columnId);

  odb::transaction t(db_->begin());

  using TQ = odb::query<Task>;
  auto q = (TQ::column_id == cid);
  if (!includeArchived)
    q = q && (TQ::archived == false);

  std::vector<core::TaskDto> out;
  for (const auto &task : db_->query<Task>(q))
    out.push_back(to_dto(task));

  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.sortOrder != b.sortOrder)
      return a.sortOrder < b.sortOrder;
    return a.id < b.id;
  });

  t.commit();
  return out;
}

core::Id OdbKanbanApi::createTask(core::Id boardId, core::Id columnId,
                                  const std::string &title,
                                  const std::string &description) {
  if (title.empty())
    throw std::runtime_error("createTask: title is empty");

  const unsigned long bid = to_ul(boardId);
  const unsigned long cid = to_ul(columnId);

  odb::transaction t(db_->begin());

  int nextOrder = 0;
  using TQ = odb::query<Task>;
  for (const auto &task : db_->query<Task>(TQ::column_id == cid))
    nextOrder = std::max(nextOrder, task.sort_order() + 1);

  Task task(bid, cid, title, description, nextOrder);
  db_->persist(task);

  t.commit();
  return to_id(task.id());
}

void OdbKanbanApi::deleteTask(core::Id taskId) {
  const unsigned long tid = to_ul(taskId);

  odb::transaction t(db_->begin());
  db_->erase<Task>(tid);
  t.commit();
}

void OdbKanbanApi::setTasksOrderForColumn(
    core::Id columnId, const std::vector<core::Id> &orderedTaskIds) {
  const unsigned long cid = to_ul(columnId);

  odb::transaction t(db_->begin());

  for (std::size_t i = 0; i < orderedTaskIds.size(); ++i) {
    const unsigned long tid = to_ul(orderedTaskIds[i]);

    std::unique_ptr<Task> task;
    try {
      task.reset(db_->load<Task>(tid));
    } catch (const std::exception &e) {
      throw std::runtime_error(
          std::string("setTasksOrderForColumn: failed to load task: ") +
          e.what());
    }

    if (!task)
      throw std::runtime_error("setTasksOrderForColumn: task not found");

    task->column_id(cid);
    task->sort_order(static_cast<int>(i));
    db_->update(*task);
  }

  t.commit();
}
} // namespace db