#pragma once

#include <QMainWindow>
#include <map>
#include <memory>

class QListWidgetItem; 

namespace odb { namespace sqlite { class database; } }

class QComboBox;
class QPushButton;
class QScrollArea;
class QWidget;
class QHBoxLayout;

class ColumnListWidget;
class KanbanColumnWidget;

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

  void loadBoardsIntoCombo();
  void reloadBoardView();

  void reloadCategoriesForBoard(unsigned long boardId);

  void addBoard();
  void deleteCurrentBoard();

  void addColumnToCurrentBoard();
  void deleteColumn(unsigned long columnId);

  void addTaskToColumn(unsigned long columnId);
  void deleteTask(unsigned long taskId);

  void editTask(unsigned long taskId);

  void renumberTasksInList(unsigned long columnId, ColumnListWidget* list);
  void onTaskMoved(unsigned long taskId, unsigned long fromColumnId, unsigned long toColumnId);

  void onColumnMoveRequested(unsigned long fromColumnId, unsigned long toColumnId, bool insertBefore);
  void persistColumnsOrder();

  void applyTaskStyle(QListWidgetItem* item) const;
  QString makeTaskToolTip(const QString& desc, qulonglong categoryId) const;

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

  // categoryId -> color/name for current board
  std::map<unsigned long, QString> categoryColors_;
  std::map<unsigned long, QString> categoryNames_;
};