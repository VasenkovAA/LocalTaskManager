#include "gui/main_window.hxx"

#include "gui/column_list_widget.hxx"
#include "gui/kanban_column_widget.hxx"
#include "gui/task_edit_dialog.hxx"

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <memory>
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

static QColor safeColor(const QString& s)
{
  QColor c(s);
  if (!c.isValid())
    return QColor();
  return c;
}

static QColor contrastTextColor(const QColor& bg)
{
  // простая оценка яркости
  const int r = bg.red();
  const int g = bg.green();
  const int b = bg.blue();
  const int y = (r * 299 + g * 587 + b * 114) / 1000;
  return (y < 128) ? Qt::white : Qt::black;
}

MainWindow::MainWindow(std::shared_ptr<odb::sqlite::database> db, QWidget* parent)
  : QMainWindow(parent), db_(std::move(db))
{
  setupUI();

  if (!db_)
  {
    QMessageBox::critical(this, "Database Error", "Database pointer is null");
    return;
  }

  loadBoardsIntoCombo();
  reloadBoardView();
}

void MainWindow::setupUI()
{
  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);

  auto* topBar = new QHBoxLayout();

  boardCombo_ = new QComboBox(this);

  addBoardBtn_ = new QPushButton("Add board", this);
  delBoardBtn_ = new QPushButton("Delete board", this);

  addColumnBtn_ = new QPushButton("Add column", this);
  refreshBtn_ = new QPushButton("Refresh", this);

  topBar->addWidget(new QLabel("Board:", this));
  topBar->addWidget(boardCombo_, 1);
  topBar->addWidget(addBoardBtn_);
  topBar->addWidget(delBoardBtn_);
  topBar->addSpacing(12);
  topBar->addWidget(addColumnBtn_);
  topBar->addSpacing(12);
  topBar->addWidget(refreshBtn_);

  root->addLayout(topBar);

  scrollArea_ = new QScrollArea(this);
  scrollArea_->setWidgetResizable(true);

  columnsHost_ = new QWidget(scrollArea_);
  columnsLayout_ = new QHBoxLayout(columnsHost_);
  columnsLayout_->setContentsMargins(8, 8, 8, 8);
  columnsLayout_->setSpacing(12);

  scrollArea_->setWidget(columnsHost_);
  root->addWidget(scrollArea_, 1);

  setCentralWidget(central);
  setWindowTitle("Kanban Task Manager");
  resize(1100, 650);

  connect(refreshBtn_, &QPushButton::clicked, this, [this] { reloadAll(); });

  connect(boardCombo_,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, [this](int) { reloadBoardView(); });

  connect(addBoardBtn_, &QPushButton::clicked, this, [this] { addBoard(); });
  connect(delBoardBtn_, &QPushButton::clicked, this, [this] { deleteCurrentBoard(); });
  connect(addColumnBtn_, &QPushButton::clicked, this, [this] { addColumnToCurrentBoard(); });
}

unsigned long MainWindow::currentBoardId() const
{
  if (boardCombo_->currentIndex() < 0)
    return 0;
  return static_cast<unsigned long>(boardCombo_->currentData().toULongLong());
}

void MainWindow::reloadAll()
{
  loadBoardsIntoCombo();
  reloadBoardView();
}

void MainWindow::clearColumnsUI()
{
  columnLists_.clear();

  while (QLayoutItem* item = columnsLayout_->takeAt(0))
  {
    if (QWidget* w = item->widget())
      w->deleteLater();
    delete item;
  }
}

void MainWindow::reloadCategoriesForBoard(unsigned long boardId)
{
  categoryColors_.clear();
  categoryNames_.clear();

  if (boardId == 0 || !db_)
    return;

  odb::transaction t(db_->begin());
  using CQ = odb::query<TaskCategory>;

  std::vector<TaskCategory> cats;
  for (const auto& c : db_->query<TaskCategory>(CQ::board_id == boardId))
    cats.push_back(c);

  std::sort(cats.begin(), cats.end(), [](const TaskCategory& a, const TaskCategory& b)
  {
    if (a.sort_order() != b.sort_order())
      return a.sort_order() < b.sort_order();
    return a.id() < b.id();
  });

  for (const auto& c : cats)
  {
    categoryNames_[c.id()] = QString::fromStdString(c.name());
    categoryColors_[c.id()] = QString::fromStdString(c.color());
  }

  t.commit();
}

QString MainWindow::makeTaskToolTip(const QString& desc, qulonglong categoryId) const
{
  QString catLine;
  if (categoryId != 0)
  {
    auto it = categoryNames_.find(static_cast<unsigned long>(categoryId));
    catLine = (it != categoryNames_.end()) ? it->second : QString("id=%1").arg(categoryId);
  }
  else
  {
    catLine = "None";
  }

  QString out;
  out += QString("Category: %1").arg(catLine);
  if (!desc.trimmed().isEmpty())
    out += "\n\n" + desc.trimmed();
  return out;
}

void MainWindow::applyTaskStyle(QListWidgetItem* item) const
{
  if (!item)
    return;

  const qulonglong catId = item->data(kRoleCategoryId).toULongLong();
  if (catId == 0)
  {
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    return;
  }

  const auto it = categoryColors_.find(static_cast<unsigned long>(catId));
  if (it == categoryColors_.end())
  {
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    return;
  }

  const QColor bg = safeColor(it->second);
  if (!bg.isValid())
  {
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    return;
  }

  item->setBackground(QBrush(bg));
  item->setForeground(QBrush(contrastTextColor(bg)));
}

void MainWindow::loadBoardsIntoCombo()
{
  boardCombo_->blockSignals(true);
  const unsigned long prev = currentBoardId();
  boardCombo_->clear();

  odb::transaction t(db_->begin());
  std::vector<KanbanBoard> boards;
  for (const auto& b : db_->query<KanbanBoard>())
    boards.push_back(b);
  t.commit();

  if (boards.empty())
  {
    boardCombo_->addItem("No boards", QVariant::fromValue<qulonglong>(0));
    boardCombo_->blockSignals(false);
    return;
  }

  std::sort(boards.begin(), boards.end(),
            [](const KanbanBoard& a, const KanbanBoard& b) { return a.id() < b.id(); });

  int selectIndex = 0;
  for (int i = 0; i < (int)boards.size(); ++i)
  {
    const auto& b = boards[i];
    boardCombo_->addItem(QString::fromStdString(b.name()), QVariant::fromValue<qulonglong>(b.id()));
    if (b.id() == prev)
      selectIndex = i;
  }
  boardCombo_->setCurrentIndex(selectIndex);
  boardCombo_->blockSignals(false);
}

void MainWindow::reloadBoardView()
{
  clearColumnsUI();

  const unsigned long boardId = currentBoardId();
  if (!db_ || boardId == 0)
  {
    columnsLayout_->addWidget(new QLabel("No board selected.", columnsHost_));
    columnsLayout_->addStretch(1);
    return;
  }

  try
  {
    reloadCategoriesForBoard(boardId);

    odb::transaction t(db_->begin());

    using ColQ = odb::query<KanbanColumn>;
    auto colsRes = db_->query<KanbanColumn>(ColQ::board_id == boardId);

    std::vector<KanbanColumn> cols;
    for (const auto& c : colsRes)
      cols.push_back(c);

    std::sort(cols.begin(), cols.end(),
              [](const KanbanColumn& a, const KanbanColumn& b)
              {
                if (a.sort_order() != b.sort_order())
                  return a.sort_order() < b.sort_order();
                return a.id() < b.id();
              });

    if (cols.empty())
    {
      columnsLayout_->addWidget(new QLabel("No columns. Add one.", columnsHost_));
      columnsLayout_->addStretch(1);
      t.commit();
      return;
    }

    for (const auto& col : cols)
    {
      auto* colW = new KanbanColumnWidget(boardId,
                                          col.id(),
                                          QString::fromStdString(col.name()),
                                          columnsHost_);

      columnsLayout_->addWidget(colW);

      ColumnListWidget* list = colW->list();
      columnLists_[col.id()] = list;

      connect(colW, &KanbanColumnWidget::addTaskRequested, this,
              [this](unsigned long colId){ addTaskToColumn(colId); });

      connect(colW, &KanbanColumnWidget::deleteColumnRequested, this,
              [this](unsigned long colId){ deleteColumn(colId); });

      connect(colW, &KanbanColumnWidget::columnMoveRequested, this,
              [this](unsigned long fromId, unsigned long toId, bool before)
              { onColumnMoveRequested(fromId, toId, before); });

      connect(list, &ColumnListWidget::taskDeleteRequested, this,
              [this](qulonglong taskId, unsigned long) { deleteTask(static_cast<unsigned long>(taskId)); });

      connect(list, &ColumnListWidget::taskMoved, this,
              [this](qulonglong taskId, unsigned long fromColumnId, unsigned long toColumnId)
              { onTaskMoved(static_cast<unsigned long>(taskId), fromColumnId, toColumnId); });

      connect(list, &ColumnListWidget::taskEditRequested, this,
              [this](qulonglong taskId) { editTask(static_cast<unsigned long>(taskId)); });
    }

    columnsLayout_->addStretch(1);

    using TaskQ = odb::query<Task>;
    auto tasksRes = db_->query<Task>((TaskQ::board_id == boardId) && (TaskQ::archived == false));

    std::vector<Task> tasks;
    for (const auto& task : tasksRes)
      tasks.push_back(task);

    std::sort(tasks.begin(), tasks.end(),
              [](const Task& a, const Task& b)
              {
                if (a.column_id() != b.column_id())
                  return a.column_id() < b.column_id();
                if (a.sort_order() != b.sort_order())
                  return a.sort_order() < b.sort_order();
                return a.id() < b.id();
              });

    for (const auto& task : tasks)
    {
      auto it = columnLists_.find(task.column_id());
      if (it == columnLists_.end() || it->second == nullptr)
        continue;

      auto* item = new QListWidgetItem(QString::fromStdString(task.title()));
      item->setData(kRoleTaskId, QVariant::fromValue<qulonglong>(task.id()));

      qulonglong catId = 0;
      if (!task.category_id().null())
        catId = static_cast<qulonglong>(task.category_id().get());
      item->setData(kRoleCategoryId, QVariant::fromValue<qulonglong>(catId));

      item->setToolTip(makeTaskToolTip(QString::fromStdString(task.description()), catId));
      applyTaskStyle(item);

      it->second->addItem(item);
    }

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error",
                          QString("Failed to load board view: %1").arg(e.what()));
  }
}

void MainWindow::addBoard()
{
  bool ok = false;
  QString name = QInputDialog::getText(this, "Add board", "Board name:", QLineEdit::Normal, "", &ok);
  if (!ok)
    return;
  name = name.trimmed();
  if (name.isEmpty())
    return;

  try
  {
    odb::transaction t(db_->begin());
    KanbanBoard b(name.toStdString());
    db_->persist(b);
    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to add board: %1").arg(e.what()));
    return;
  }

  reloadAll();
}

void MainWindow::deleteCurrentBoard()
{
  const unsigned long boardId = currentBoardId();
  if (boardId == 0)
    return;

  const auto reply = QMessageBox::question(this, "Delete board",
                                          "Delete board and all its columns/tasks?",
                                          QMessageBox::Yes | QMessageBox::No);
  if (reply != QMessageBox::Yes)
    return;

  try
  {
    odb::transaction t(db_->begin());

    using TQ = odb::query<Task>;
    for (const auto& task : db_->query<Task>(TQ::board_id == boardId))
      db_->erase<Task>(task.id());

    using CQ = odb::query<KanbanColumn>;
    for (const auto& col : db_->query<KanbanColumn>(CQ::board_id == boardId))
      db_->erase<KanbanColumn>(col.id());

    using CatQ = odb::query<TaskCategory>;
    for (const auto& c : db_->query<TaskCategory>(CatQ::board_id == boardId))
      db_->erase<TaskCategory>(c.id());

    db_->erase<KanbanBoard>(boardId);

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to delete board: %1").arg(e.what()));
    return;
  }

  reloadAll();
}

void MainWindow::addColumnToCurrentBoard()
{
  const unsigned long boardId = currentBoardId();
  if (boardId == 0)
    return;

  bool ok = false;
  QString name = QInputDialog::getText(this, "Add column", "Column name:", QLineEdit::Normal, "", &ok);
  if (!ok)
    return;
  name = name.trimmed();
  if (name.isEmpty())
    return;

  try
  {
    odb::transaction t(db_->begin());

    int maxOrder = -1;
    using CQ = odb::query<KanbanColumn>;
    for (const auto& c : db_->query<KanbanColumn>(CQ::board_id == boardId))
      maxOrder = std::max(maxOrder, c.sort_order());

    KanbanColumn col(boardId, name.toStdString(), maxOrder + 1);
    db_->persist(col);

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to add column: %1").arg(e.what()));
    return;
  }

  reloadBoardView();
}

void MainWindow::deleteColumn(unsigned long columnId)
{
  if (columnId == 0)
    return;

  const auto reply = QMessageBox::question(this, "Delete column",
                                          "Delete column and all tasks in it?",
                                          QMessageBox::Yes | QMessageBox::No);
  if (reply != QMessageBox::Yes)
    return;

  try
  {
    odb::transaction t(db_->begin());

    using TQ = odb::query<Task>;
    for (const auto& task : db_->query<Task>(TQ::column_id == columnId))
      db_->erase<Task>(task.id());

    db_->erase<KanbanColumn>(columnId);

    const unsigned long boardId = currentBoardId();
    using CQ = odb::query<KanbanColumn>;
    std::vector<KanbanColumn> cols;
    for (const auto& c : db_->query<KanbanColumn>(CQ::board_id == boardId))
      cols.push_back(c);

    std::sort(cols.begin(), cols.end(),
              [](const KanbanColumn& a, const KanbanColumn& b)
              {
                if (a.sort_order() != b.sort_order())
                  return a.sort_order() < b.sort_order();
                return a.id() < b.id();
              });

    for (int i = 0; i < (int)cols.size(); ++i)
    {
      auto c = cols[i];
      c.sort_order(i);
      db_->update(c);
    }

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to delete column: %1").arg(e.what()));
    return;
  }

  reloadBoardView();
}

void MainWindow::addTaskToColumn(unsigned long columnId)
{
  const unsigned long boardId = currentBoardId();
  if (boardId == 0 || columnId == 0)
    return;

  bool ok = false;
  QString title = QInputDialog::getText(this, "Add task", "Title:", QLineEdit::Normal, "", &ok);
  if (!ok)
    return;
  title = title.trimmed();
  if (title.isEmpty())
    return;

  QString desc = QInputDialog::getMultiLineText(this, "Add task", "Description:", "", &ok);
  if (!ok)
    desc = "";

  try
  {
    odb::transaction t(db_->begin());

    int nextOrder = 0;
    using TQ = odb::query<Task>;
    for (const auto& task : db_->query<Task>(TQ::column_id == columnId))
      nextOrder = std::max(nextOrder, task.sort_order() + 1);

    Task task(boardId, columnId, title.toStdString(), desc.toStdString(), nextOrder);
    db_->persist(task);

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to add task: %1").arg(e.what()));
    return;
  }

  reloadBoardView();
}

void MainWindow::deleteTask(unsigned long taskId)
{
  if (taskId == 0)
    return;

  try
  {
    odb::transaction t(db_->begin());
    db_->erase<Task>(taskId);
    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to delete task: %1").arg(e.what()));
    return;
  }

  reloadBoardView();
}

void MainWindow::editTask(unsigned long taskId)
{
  if (!db_ || taskId == 0)
    return;

  auto edited = TaskEditDialog::editTask(db_, taskId, this);
  if (!edited.has_value())
    return;

  // найти элемент в UI
  QListWidgetItem* foundItem = nullptr;
  ColumnListWidget* foundList = nullptr;

  for (auto& [colId, list] : columnLists_)
  {
    if (!list) continue;
    if (auto* it = list->findItemByTaskId(taskId))
    {
      foundItem = it;
      foundList = list;
      (void)colId;
      break;
    }
  }

  if (!foundItem || !foundList)
  {
    // если не нашли — проще перезагрузить
    reloadBoardView();
    return;
  }

  foundItem->setText(edited->title);

  const qulonglong newCatId = edited->categoryId.has_value()
    ? static_cast<qulonglong>(*edited->categoryId)
    : 0;

  foundItem->setData(kRoleCategoryId, QVariant::fromValue<qulonglong>(newCatId));
  foundItem->setToolTip(makeTaskToolTip(edited->description, newCatId));
  reloadCategoriesForBoard(currentBoardId());
  applyTaskStyle(foundItem);
}

void MainWindow::renumberTasksInList(unsigned long columnId, ColumnListWidget* list)
{
  if (!list)
    return;

  for (int row = 0; row < list->count(); ++row)
  {
    auto* it = list->item(row);
    if (!it)
      continue;

    const unsigned long taskId = static_cast<unsigned long>(it->data(kRoleTaskId).toULongLong());
    std::unique_ptr<Task> task(db_->load<Task>(taskId));
    task->column_id(columnId);
    task->sort_order(row);
    db_->update(*task);
  }
}

void MainWindow::onTaskMoved(unsigned long taskId, unsigned long fromColumnId, unsigned long toColumnId)
{
  try
  {
    odb::transaction t(db_->begin());

    ColumnListWidget* fromList = nullptr;
    ColumnListWidget* toList = nullptr;

    if (auto it = columnLists_.find(fromColumnId); it != columnLists_.end())
      fromList = it->second;

    if (auto it = columnLists_.find(toColumnId); it != columnLists_.end())
      toList = it->second;

    if (fromList)
      renumberTasksInList(fromColumnId, fromList);
    if (toList && toList != fromList)
      renumberTasksInList(toColumnId, toList);

    t.commit();

    // обновить стиль карточки в новом месте (цвет берется из данных item)
    if (toList)
    {
      if (auto* it = toList->findItemByTaskId(taskId))
        applyTaskStyle(it);
    }
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to move task: %1").arg(e.what()));
    reloadBoardView();
  }
}

void MainWindow::onColumnMoveRequested(unsigned long fromColumnId, unsigned long toColumnId, bool insertBefore)
{
  if (!columnsLayout_ || fromColumnId == 0 || toColumnId == 0 || fromColumnId == toColumnId)
    return;

  // найти индексы виджетов колонок в layout (игнорируем stretch в конце)
  const int count = columnsLayout_->count();
  int fromIndex = -1;
  int toIndex = -1;

  for (int i = 0; i < count; ++i)
  {
    auto* w = columnsLayout_->itemAt(i) ? columnsLayout_->itemAt(i)->widget() : nullptr;
    auto* colW = qobject_cast<KanbanColumnWidget*>(w);
    if (!colW) continue;

    if (colW->columnId() == fromColumnId)
      fromIndex = i;
    if (colW->columnId() == toColumnId)
      toIndex = i;
  }

  if (fromIndex < 0 || toIndex < 0)
    return;

  int insertIndex = insertBefore ? toIndex : (toIndex + 1);
  if (fromIndex < insertIndex)
    insertIndex -= 1;

  QLayoutItem* taken = columnsLayout_->takeAt(fromIndex);
  if (!taken)
    return;

  QWidget* w = taken->widget();
  delete taken;

  if (!w)
    return;

  columnsLayout_->insertWidget(insertIndex, w);

  persistColumnsOrder();
}

void MainWindow::persistColumnsOrder()
{
  try
  {
    odb::transaction t(db_->begin());

    int order = 0;
    for (int i = 0; i < columnsLayout_->count(); ++i)
    {
      auto* w = columnsLayout_->itemAt(i) ? columnsLayout_->itemAt(i)->widget() : nullptr;
      auto* colW = qobject_cast<KanbanColumnWidget*>(w);
      if (!colW) continue;

      const unsigned long colId = colW->columnId();
      std::unique_ptr<KanbanColumn> col(db_->load<KanbanColumn>(colId));
      if (!col)
        continue;

      col->sort_order(order++);
      db_->update(*col);
    }

    t.commit();
  }
  catch (const std::exception& e)
  {
    QMessageBox::critical(this, "Database Error", QString("Failed to reorder columns: %1").arg(e.what()));
    reloadBoardView();
  }
}