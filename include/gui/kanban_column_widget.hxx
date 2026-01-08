#pragma once

#include <QWidget>
#include <QPoint>
#include <QApplication>
#include <QDragMoveEvent>

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;

class ColumnListWidget;

static constexpr const char* kColumnMime = "application/x-localtaskmanager-column";

class KanbanColumnWidget final : public QWidget
{
  Q_OBJECT
public:
  explicit KanbanColumnWidget(unsigned long boardId,
                              unsigned long columnId,
                              const QString& name,
                              QWidget* parent = nullptr);

  unsigned long boardId() const { return boardId_; }
  unsigned long columnId() const { return columnId_; }

  ColumnListWidget* list() const { return list_; }
  void setTitle(const QString& t);

signals:
  void addTaskRequested(unsigned long columnId);
  void deleteColumnRequested(unsigned long columnId);
  void columnMoveRequested(unsigned long fromColumnId, unsigned long toColumnId, bool insertBefore);

public:
  void startColumnDrag();
  void handleColumnDrop(unsigned long fromColumnId, bool insertBefore);

private:
  unsigned long boardId_ = 0;
  unsigned long columnId_ = 0;

  QWidget* header_ = nullptr;
  QLabel* title_ = nullptr;
  ColumnListWidget* list_ = nullptr;
};