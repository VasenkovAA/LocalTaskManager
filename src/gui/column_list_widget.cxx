#include "gui/column_list_widget.hxx"

#include <QAbstractItemView>
#include <QDataStream>
#include <QDrag>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QPixmap>
#include <QVariant>

ColumnListWidget::ColumnListWidget(unsigned long columnId, QWidget* parent)
  : QListWidget(parent), columnId_(columnId)
{
  setSelectionMode(QAbstractItemView::SingleSelection);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);
  setDragDropMode(QAbstractItemView::DragDrop);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QListWidget::customContextMenuRequested, this, &ColumnListWidget::onContextMenu);
}

unsigned long ColumnListWidget::columnId() const
{
  return columnId_;
}

QListWidgetItem* ColumnListWidget::findItemByTaskId(qulonglong taskId) const
{
  for (int i = 0; i < count(); ++i)
  {
    auto* it = item(i);
    if (it && it->data(Qt::UserRole).toULongLong() == taskId)
      return it;
  }
  return nullptr;
}

QStringList ColumnListWidget::mimeTypes() const
{
  return {QString::fromLatin1(kTaskMime)};
}

QMimeData* ColumnListWidget::mimeData(const QList<QListWidgetItem*> items) const
{
  if (items.empty() || items.front() == nullptr)
    return nullptr;

  auto* it = items.front();
  const qulonglong taskId = it->data(Qt::UserRole).toULongLong();
  const QString title = it->text();

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_15);

  out << taskId;
  out << static_cast<qulonglong>(columnId_);
  out << title;

  auto* md = new QMimeData();
  md->setData(QString::fromLatin1(kTaskMime), bytes);
  return md;
}

void ColumnListWidget::startDrag(Qt::DropActions supportedActions)
{
  auto* it = currentItem();
  if (!it)
    return;

  auto* drag = new QDrag(this);
  drag->setMimeData(mimeData({it}));

  QPixmap pm(viewport()->visibleRegion().boundingRect().size());
  pm.fill(Qt::transparent);
  drag->setPixmap(pm);

  drag->exec(supportedActions, Qt::MoveAction);
}

void ColumnListWidget::dropEvent(QDropEvent* event)
{
  if (!event || !event->mimeData() ||
      !event->mimeData()->hasFormat(QString::fromLatin1(kTaskMime)))
  {
    event->ignore();
    return;
  }

  QByteArray bytes = event->mimeData()->data(QString::fromLatin1(kTaskMime));
  QDataStream in(&bytes, QIODevice::ReadOnly);
  in.setVersion(QDataStream::Qt_5_15);

  qulonglong taskId = 0;
  qulonglong fromCol = 0;
  QString title;
  in >> taskId;
  in >> fromCol;
  in >> title;

  auto* src = qobject_cast<ColumnListWidget*>(event->source());
  const unsigned long fromColumnId = static_cast<unsigned long>(fromCol);
  const unsigned long toColumnId = columnId_;

  int dropRow = indexAt(event->pos()).row();
  if (dropRow < 0)
    dropRow = count();

  if (src == this)
  {
    auto* existing = findItemByTaskId(taskId);
    if (!existing)
    {
      event->ignore();
      return;
    }

    int fromRow = row(existing);
    if (fromRow == dropRow || fromRow == dropRow - 1)
    {
      event->acceptProposedAction();
      return;
    }

    auto* taken = takeItem(fromRow);
    if (!taken)
    {
      event->ignore();
      return;
    }

    int insertRow = dropRow;
    if (fromRow < insertRow)
      insertRow -= 1;

    insertItem(insertRow, taken);
    setCurrentItem(taken);
  }
  else
  {
    auto* newItem = new QListWidgetItem(title);
    newItem->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(taskId));
    insertItem(dropRow, newItem);
    setCurrentItem(newItem);

    if (src)
    {
      if (auto* old = src->findItemByTaskId(taskId))
        delete src->takeItem(src->row(old));
    }
  }

  event->setDropAction(Qt::MoveAction);
  event->accept();

  emit taskMoved(taskId, fromColumnId, toColumnId);
}

void ColumnListWidget::onContextMenu(const QPoint& pos)
{
  auto* it = itemAt(pos);
  if (!it)
    return;

  const qulonglong taskId = it->data(Qt::UserRole).toULongLong();

  QMenu menu(this);
  QAction* del = menu.addAction("Delete task");
  QAction* chosen = menu.exec(mapToGlobal(pos));
  if (chosen == del)
    emit taskDeleteRequested(taskId, columnId_);
}