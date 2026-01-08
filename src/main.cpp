#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "app/settings.hxx"
#include "db/database.hxx"
#include "db/odb_kanban_api.hxx"

// ---------------------
// Helpers: printing
// ---------------------
static void dump(const std::vector<core::BoardDto>& boards)
{
  std::cout << "Boards:\n";
  for (const auto& b : boards)
    std::cout << "  [" << b.id << "] " << b.name << " -- " << b.description << "\n";
}

static void dump(const std::vector<core::ColumnDto>& cols)
{
  std::cout << "Columns:\n";
  for (const auto& c : cols)
    std::cout << "  [" << c.id << "] board=" << c.boardId << " order=" << c.sortOrder
              << " name=" << c.name << "\n";
}

static void dump(const std::vector<core::CategoryDto>& cats)
{
  std::cout << "Categories:\n";
  for (const auto& c : cats)
    std::cout << "  [" << c.id << "] board=" << c.boardId << " order=" << c.sortOrder
              << " name=" << c.name << " color=" << c.color << "\n";
}

static void dump(const std::vector<core::TaskDto>& tasks)
{
  std::cout << "Tasks:\n";
  for (const auto& t : tasks)
  {
    std::cout << "  [" << t.id << "] board=" << t.boardId << " col=" << t.columnId
              << " order=" << t.sortOrder << " title=" << t.title;

    if (t.categoryId.has_value())
      std::cout << " category=" << *t.categoryId;
    else
      std::cout << " category=NULL";

    if (t.archived)
      std::cout << " (archived)";

    std::cout << "\n";
  }
}

static void require(bool ok, const char* msg)
{
  if (!ok)
    throw std::runtime_error(std::string("TEST FAILED: ") + msg);
}

static bool containsBoard(const std::vector<core::BoardDto>& v, core::Id id)
{
  return std::any_of(v.begin(), v.end(), [&](const auto& b) { return b.id == id; });
}

static bool containsColumn(const std::vector<core::ColumnDto>& v, core::Id id)
{
  return std::any_of(v.begin(), v.end(), [&](const auto& c) { return c.id == id; });
}

static bool containsCategory(const std::vector<core::CategoryDto>& v, core::Id id)
{
  return std::any_of(v.begin(), v.end(), [&](const auto& c) { return c.id == id; });
}

static std::vector<core::Id> idsFromTasks(const std::vector<core::TaskDto>& tasks)
{
  std::vector<core::Id> out;
  out.reserve(tasks.size());
  for (const auto& t : tasks)
    out.push_back(t.id);
  return out;
}

// ---------------------
// Main test scenario
// ---------------------
int main(int argc, char* argv[])
{
  try
  {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("LocalTaskManager");
    QCoreApplication::setApplicationName("LocalTaskManager");

    QCommandLineParser parser;
    parser.setApplicationDescription("LocalTaskManager (console test: boards/columns/tasks/categories)");
    parser.addHelpOption();

    QCommandLineOption configOpt(QStringList{"c", "config"},
                                 "Path to ini config file (default: рядом с программой).",
                                 "path");
    QCommandLineOption dbOpt(QStringList{"d", "db"},
                             "Path to sqlite database file. If set, it will be saved to ini.",
                             "path");

    QCommandLineOption cleanupOpt(QStringList{"cleanup"},
                                  "Delete created test board at the end (useful for CI).");

    parser.addOption(configOpt);
    parser.addOption(dbOpt);
    parser.addOption(cleanupOpt);
    parser.process(app);

    QString iniPath = parser.value(configOpt).trimmed();
    if (iniPath.isEmpty())
      iniPath = defaultIniPath();

    const QString cliDbPath = parser.value(dbOpt).trimmed();

    const AppSettings st = loadOrCreateSettings(iniPath, cliDbPath);
    auto opened = db::openDatabaseReadOrCreate(st.dbPath);

    // Практичнее вызывать всегда (seedIfEmpty сам проверит пустоту).
    db::ensureSchemaBestEffort(*opened.db);
    db::seedIfEmpty(*opened.db);

    db::OdbKanbanApi api(opened.db);

    // ---------------------
    // 1) List boards
    // ---------------------
    std::cout << "\n=== Initial state ===\n";
    auto boards = api.listBoards();
    dump(boards);

    // ---------------------
    // 2) Create board
    // ---------------------
    std::cout << "\n=== Create test board ===\n";
    const core::Id testBoardId = api.createBoard("Console Test Board", "Board created by console test");
    boards = api.listBoards();
    dump(boards);
    require(containsBoard(boards, testBoardId), "Created board not found in listBoards()");

    // ---------------------
    // 3) Columns: create/list/delete
    // ---------------------
    std::cout << "\n=== Create columns ===\n";
    const core::Id todoColId  = api.createColumn(testBoardId, "Todo");
    const core::Id doingColId = api.createColumn(testBoardId, "Doing");

    auto cols = api.listColumns(testBoardId);
    dump(cols);
    require(containsColumn(cols, todoColId), "Todo column not found after createColumn()");
    require(containsColumn(cols, doingColId), "Doing column not found after createColumn()");

    // ---------------------
    // 4) Categories: create/list
    // ---------------------
    std::cout << "\n=== Create categories ===\n";
    const core::Id bugCatId     = api.createCategory(testBoardId, "Bug", "#e74c3c");
    const core::Id featureCatId = api.createCategory(testBoardId, "Feature", "#3498db");

    auto cats = api.listCategories(testBoardId);
    dump(cats);
    require(containsCategory(cats, bugCatId), "Bug category not found after createCategory()");
    require(containsCategory(cats, featureCatId), "Feature category not found after createCategory()");

    // ---------------------
    // 5) Tasks: create/list
    // ---------------------
    std::cout << "\n=== Create tasks ===\n";
    const core::Id tA = api.createTask(testBoardId, todoColId, "Task A", "from console");
    const core::Id tB = api.createTask(testBoardId, todoColId, "Task B", "");
    const core::Id tC = api.createTask(testBoardId, todoColId, "Task C", "");
    const core::Id tD = api.createTask(testBoardId, doingColId, "Task D", "initially in Doing");

    auto todoTasks = api.listTasksForColumn(todoColId, false);
    auto doingTasks = api.listTasksForColumn(doingColId, false);

    std::cout << "\nTodo column tasks:\n";
    dump(todoTasks);
    std::cout << "\nDoing column tasks:\n";
    dump(doingTasks);

    require(std::any_of(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tA; }), "Task A not in Todo");
    require(std::any_of(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tB; }), "Task B not in Todo");
    require(std::any_of(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tC; }), "Task C not in Todo");
    require(std::any_of(doingTasks.begin(), doingTasks.end(), [&](const auto& t){ return t.id == tD; }), "Task D not in Doing");

    // ---------------------
    // 6) Assign/clear task category
    // ---------------------
    std::cout << "\n=== Set category Bug for Task B ===\n";
    api.setTaskCategory(tB, bugCatId);

    todoTasks = api.listTasksForColumn(todoColId, false);
    dump(todoTasks);

    {
      auto it = std::find_if(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tB; });
      require(it != todoTasks.end(), "Task B not found after setTaskCategory()");
      require(it->categoryId.has_value() && *it->categoryId == bugCatId, "Task B category not set to Bug");
    }

    std::cout << "\n=== Clear category for Task B ===\n";
    api.setTaskCategory(tB, std::nullopt);

    todoTasks = api.listTasksForColumn(todoColId, false);
    dump(todoTasks);

    {
      auto it = std::find_if(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tB; });
      require(it != todoTasks.end(), "Task B not found after clear category");
      require(!it->categoryId.has_value(), "Task B category not cleared");
    }

    // ---------------------
    // 7) Reorder tasks in Todo
    // ---------------------
    std::cout << "\n=== Reorder tasks in Todo: (C, A, B) ===\n";
    api.setTasksOrderForColumn(todoColId, {tC, tA, tB});

    todoTasks = api.listTasksForColumn(todoColId, false);
    dump(todoTasks);

    // Проверим порядок
    require(todoTasks.size() >= 3, "Todo tasks < 3 after reorder");
    require(todoTasks[0].id == tC, "Expected Task C at position 0 after reorder");
    require(todoTasks[1].id == tA, "Expected Task A at position 1 after reorder");
    require(todoTasks[2].id == tB, "Expected Task B at position 2 after reorder");

    // ---------------------
    // 8) Move task between columns using setTasksOrderForColumn
    //    Move Task B from Todo -> Doing, keep Doing order: (D, B)
    // ---------------------
    std::cout << "\n=== Move Task B Todo -> Doing (via setTasksOrderForColumn) ===\n";
    api.setTasksOrderForColumn(doingColId, {tD, tB});
    api.setTasksOrderForColumn(todoColId, {tC, tA}); // Todo without B

    todoTasks = api.listTasksForColumn(todoColId, false);
    doingTasks = api.listTasksForColumn(doingColId, false);

    std::cout << "\nTodo column tasks after move:\n";
    dump(todoTasks);
    std::cout << "\nDoing column tasks after move:\n";
    dump(doingTasks);

    require(std::none_of(todoTasks.begin(), todoTasks.end(), [&](const auto& t){ return t.id == tB; }), "Task B still in Todo after move");
    require(std::any_of(doingTasks.begin(), doingTasks.end(), [&](const auto& t){ return t.id == tB; }), "Task B not in Doing after move");

    // ---------------------
    // 9) listTasksForBoard (both includeArchived false/true)
    // ---------------------
    std::cout << "\n=== List tasks for board (includeArchived=false) ===\n";
    auto boardTasks = api.listTasksForBoard(testBoardId, false);
    dump(boardTasks);

    std::cout << "\n=== List tasks for board (includeArchived=true) ===\n";
    auto boardTasksAll = api.listTasksForBoard(testBoardId, true);
    dump(boardTasksAll);

    // ---------------------
    // 10) deleteTask
    // ---------------------
    std::cout << "\n=== Delete Task D ===\n";
    api.deleteTask(tD);

    doingTasks = api.listTasksForColumn(doingColId, false);
    dump(doingTasks);
    require(std::none_of(doingTasks.begin(), doingTasks.end(), [&](const auto& t){ return t.id == tD; }), "Task D still present after deleteTask()");

    // ---------------------
    // 11) deleteCategory (should also clear tasks which reference it)
    // ---------------------
    std::cout << "\n=== Delete category Bug ===\n";
    api.deleteCategory(bugCatId);

    cats = api.listCategories(testBoardId);
    dump(cats);
    require(!containsCategory(cats, bugCatId), "Bug category still present after deleteCategory()");

    // ---------------------
    // 12) deleteColumn (Doing) + verify
    // ---------------------
    std::cout << "\n=== Delete column Doing (will delete tasks in it) ===\n";
    api.deleteColumn(doingColId);

    cols = api.listColumns(testBoardId);
    dump(cols);
    require(!containsColumn(cols, doingColId), "Doing column still present after deleteColumn()");

    // ---------------------
    // 13) Optional cleanup: deleteBoard
    // ---------------------
    const bool cleanup = parser.isSet(cleanupOpt);
    if (cleanup)
    {
      std::cout << "\n=== Cleanup: delete test board ===\n";
      api.deleteBoard(testBoardId);

      boards = api.listBoards();
      dump(boards);
      require(!containsBoard(boards, testBoardId), "Test board still present after deleteBoard()");
    }

    std::cout << "\nALL TESTS OK\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
}