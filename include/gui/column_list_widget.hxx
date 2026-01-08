#pragma once

#include <QListWidget>

static constexpr const char* kTaskMime = "application/x-localtaskmanager-task";

static constexpr int kRoleTaskId     = Qt::UserRole;
static constexpr int kRoleCategoryId = Qt::UserRole + 1;

class ColumnListWidget final : public QListWidget
{
  Q_OBJECT

public:
  explicit ColumnListWidget(unsigned long columnId, QWidget* parent = nullptr);

  unsigned long columnId() const;

  QListWidgetItem* findItemByTaskId(qulonglong taskId) const;

signals:
  void taskMoved(qulonglong taskId, unsigned long fromColumnId, unsigned long toColumnId);
  void taskDeleteRequested(qulonglong taskId, unsigned long columnId);
  void taskEditRequested(qulonglong taskId);

protected:
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QList<QListWidgetItem*> items) const override;
  void startDrag(Qt::DropActions supportedActions) override;
  void dropEvent(QDropEvent* event) override;

private slots:
  void onContextMenu(const QPoint& pos);

private:
  unsigned long columnId_ = 0;
};