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

  connect(this, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it)
  {
    if (!it) return;
    const qulonglong taskId = it->data(kRoleTaskId).toULongLong();
    if (taskId != 0)
      emit taskEditRequested(taskId);
  });
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
    if (it && it->data(kRoleTaskId).toULongLong() == taskId)
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
  const qulonglong taskId = it->data(kRoleTaskId).toULongLong();
  const QString title = it->text();

  // сохраняем categoryId чтобы при переносе между колонками не терять цвет
  const qulonglong catId = it->data(kRoleCategoryId).toULongLong();

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_15);

  out << taskId;
  out << static_cast<qulonglong>(columnId_);
  out << title;
  out << catId;

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
  qulonglong catId = 0;

  in >> taskId;
  in >> fromCol;
  in >> title;
  if (!in.atEnd())
    in >> catId;

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
    // попытаться взять categoryId у исходного элемента (надежнее, чем из mime, если src есть)
    if (src)
    {
      if (auto* old = src->findItemByTaskId(taskId))
        catId = old->data(kRoleCategoryId).toULongLong();
    }

    auto* newItem = new QListWidgetItem(title);
    newItem->setData(kRoleTaskId, QVariant::fromValue<qulonglong>(taskId));
    newItem->setData(kRoleCategoryId, QVariant::fromValue<qulonglong>(catId));
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

  const qulonglong taskId = it->data(kRoleTaskId).toULongLong();

  QMenu menu(this);
  QAction* edit = menu.addAction("Open / Edit...");
  QAction* del  = menu.addAction("Delete task");

  QAction* chosen = menu.exec(mapToGlobal(pos));
  if (chosen == edit)
    emit taskEditRequested(taskId);
  else if (chosen == del)
    emit taskDeleteRequested(taskId, columnId_);
}