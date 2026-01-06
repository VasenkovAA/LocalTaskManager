#pragma once

#include <QMainWindow>
#include <map>
#include <memory>

namespace odb { namespace sqlite { class database; } }

class QComboBox;
class QPushButton;
class QScrollArea;
class QWidget;
class QHBoxLayout;

class ColumnListWidget;
class KanbanColumn;

class MainWindow final : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(std::shared_ptr<odb::sqlite::database> db, QWidget* parent = nullptr);

private:
  void setupUI();

  unsigned long currentBoardId() const;
  void reloadAll();
  void clearColumnsUI();
  QWidget* makeColumnWidget(const KanbanColumn& col);
  void loadBoardsIntoCombo();
  void reloadBoardView();

  void addBoard();
  void deleteCurrentBoard();

  void addColumnToCurrentBoard();
  void deleteColumn(unsigned long columnId);

  void addTaskToColumn(unsigned long columnId);
  void deleteTask(unsigned long taskId);
  void renumberTasksInList(unsigned long columnId, ColumnListWidget* list);
  void onTaskMoved(unsigned long taskId, unsigned long fromColumnId, unsigned long toColumnId);

private:
  std::shared_ptr<odb::sqlite::database> db_;

  QComboBox* boardCombo_ = nullptr;
  QPushButton* addBoardBtn_ = nullptr;
  QPushButton* delBoardBtn_ = nullptr;
  QPushButton* addColumnBtn_ = nullptr;
  QPushButton* refreshBtn_ = nullptr;

  QScrollArea* scrollArea_ = nullptr;
  QWidget* columnsHost_ = nullptr;
  QHBoxLayout* columnsLayout_ = nullptr;

  std::map<unsigned long, ColumnListWidget*> columnLists_;
};