#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <iostream>

#include "app/settings.hxx"
#include "db/database.hxx"
#include "db/odb_kanban_api.hxx"

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
    std::cout << "  [" << c.id << "] board=" << c.boardId << " order=" << c.sortOrder << " name=" << c.name << "\n";
}

static void dump(const std::vector<core::TaskDto>& tasks)
{
  std::cout << "Tasks:\n";
  for (const auto& t : tasks)
  {
    std::cout << "  [" << t.id << "] board=" << t.boardId << " col=" << t.columnId
              << " order=" << t.sortOrder << " title=" << t.title
              << (t.archived ? " (archived)" : "") << "\n";
  }
}

int main(int argc, char* argv[])
{
  try
  {
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("LocalTaskManager");
    QCoreApplication::setApplicationName("LocalTaskManager");

    QCommandLineParser parser;
    parser.setApplicationDescription("LocalTaskManager (console test)");
    parser.addHelpOption();

    QCommandLineOption configOpt(QStringList{"c", "config"},
                                 "Path to ini config file (default: рядом с программой).",
                                 "path");
    QCommandLineOption dbOpt(QStringList{"d", "db"},
                             "Path to sqlite database file. If set, it will be saved to ini.",
                             "path");

    parser.addOption(configOpt);
    parser.addOption(dbOpt);
    parser.process(app);

    QString iniPath = parser.value(configOpt).trimmed();
    if (iniPath.isEmpty())
      iniPath = defaultIniPath();

    const QString cliDbPath = parser.value(dbOpt).trimmed();

    const AppSettings st = loadOrCreateSettings(iniPath, cliDbPath);
    auto opened = db::openDatabaseReadOrCreate(st.dbPath);

    // Я бы делал ensureSchema всегда, но оставлю вашу логику + чуть безопаснее:
    if (opened.createdNew)
    {
      db::ensureSchemaBestEffort(*opened.db);
      db::seedIfEmpty(*opened.db);
    }

    db::OdbKanbanApi api(opened.db);

    // Простой сценарий теста:
    auto boards = api.listBoards();
    dump(boards);

    if (boards.empty())
    {
      std::cout << "\nNo boards; creating one...\n";
      const core::Id bid = api.createBoard("Console Board", "Created from console test");
      boards = api.listBoards();
      dump(boards);
      (void)bid;
    }

    const core::Id boardId = boards.front().id;

    std::cout << "\nBoard " << boardId << " columns:\n";
    auto cols = api.listColumns(boardId);
    dump(cols);

    if (cols.empty())
    {
      std::cout << "\nNo columns; creating Todo/Doing...\n";
      api.createColumn(boardId, "Todo");
      api.createColumn(boardId, "Doing");
      cols = api.listColumns(boardId);
      dump(cols);
    }

    const core::Id todoCol = cols.front().id;

    std::cout << "\nCreating 3 tasks in first column...\n";
    const core::Id t1 = api.createTask(boardId, todoCol, "Task A", "from console");
    const core::Id t2 = api.createTask(boardId, todoCol, "Task B", "");
    const core::Id t3 = api.createTask(boardId, todoCol, "Task C", "");

    auto tasksTodo = api.listTasksForColumn(todoCol, false);
    dump(tasksTodo);

    std::cout << "\nReorder tasks in column (C, A, B)...\n";
    api.setTasksOrderForColumn(todoCol, {t3, t1, t2});

    tasksTodo = api.listTasksForColumn(todoCol, false);
    dump(tasksTodo);

    std::cout << "\nAll tasks for board:\n";
    auto tasksBoard = api.listTasksForBoard(boardId, false);
    dump(tasksBoard);

    std::cout << "\nOK\n";
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
}