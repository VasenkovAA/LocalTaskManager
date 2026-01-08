#include "gui/kanban_column_widget.hxx"
#include "gui/column_list_widget.hxx"

#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

// маленькая “ручка” для старта drag колонки
class ColumnDragHandle final : public QWidget
{
public:
  explicit ColumnDragHandle(KanbanColumnWidget* owner)
    : QWidget(owner), owner_(owner)
  {
    setFixedWidth(18);
    setCursor(Qt::OpenHandCursor);
    setToolTip("Drag to move column");
  }

protected:
  void mousePressEvent(QMouseEvent* e) override
  {
    if (e && e->button() == Qt::LeftButton)
    {
      pressPos_ = e->pos();
      setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(e);
  }

  void mouseReleaseEvent(QMouseEvent* e) override
  {
    setCursor(Qt::OpenHandCursor);
    QWidget::mouseReleaseEvent(e);
  }

  void mouseMoveEvent(QMouseEvent* e) override
  {
    if (!e || !(e->buttons() & Qt::LeftButton))
      return;

    if ((e->pos() - pressPos_).manhattanLength() < QApplication::startDragDistance())
      return;

    if (owner_)
      owner_->startColumnDrag();
  }

private:
  KanbanColumnWidget* owner_ = nullptr;
  QPoint pressPos_;
};

// заголовок принимает drop колонки
class ColumnHeaderBar final : public QWidget
{
public:
  explicit ColumnHeaderBar(KanbanColumnWidget* owner)
    : QWidget(owner), owner_(owner)
  {
    setAcceptDrops(true);
  }

protected:
  void dragEnterEvent(QDragEnterEvent* e) override
  {
    if (e && e->mimeData() && e->mimeData()->hasFormat(QString::fromLatin1(kColumnMime)))
      e->acceptProposedAction();
    else
      e->ignore();
  }

  void dragMoveEvent(QDragMoveEvent* e) override
  {
    if (e && e->mimeData() && e->mimeData()->hasFormat(QString::fromLatin1(kColumnMime)))
      e->acceptProposedAction();
    else
      e->ignore();
  }

  void dropEvent(QDropEvent* e) override
  {
    if (!e || !e->mimeData() || !e->mimeData()->hasFormat(QString::fromLatin1(kColumnMime)))
    {
      e->ignore();
      return;
    }

    QByteArray bytes = e->mimeData()->data(QString::fromLatin1(kColumnMime));
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_15);

    qulonglong fromCol = 0;
    qulonglong fromBoard = 0;
    in >> fromCol;
    in >> fromBoard;

    if (!owner_)
    {
      e->ignore();
      return;
    }

    // простой “до/после” по половине заголовка
    const bool insertBefore = (e->pos().x() < (width() / 2));
    owner_->handleColumnDrop(static_cast<unsigned long>(fromCol), insertBefore);

    e->setDropAction(Qt::MoveAction);
    e->accept();
  }

private:
  KanbanColumnWidget* owner_ = nullptr;
};

KanbanColumnWidget::KanbanColumnWidget(unsigned long boardId,
                                       unsigned long columnId,
                                       const QString& name,
                                       QWidget* parent)
  : QWidget(parent), boardId_(boardId), columnId_(columnId)
{
  setMinimumWidth(280);
  setStyleSheet("background: #f6f6f6; border: 1px solid #ddd; border-radius: 6px;");

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(8);

  header_ = new ColumnHeaderBar(this);
  auto* headerRow = new QHBoxLayout(header_);
  headerRow->setContentsMargins(0, 0, 0, 0);
  headerRow->setSpacing(6);

  auto* handle = new ColumnDragHandle(this);
  handle->setStyleSheet("background: transparent; border: none;");

  title_ = new QLabel(name, header_);
  title_->setStyleSheet("font-weight: 600; font-size: 14px;");

  auto* addTaskBtn = new QPushButton("+", header_);
  addTaskBtn->setFixedWidth(32);

  auto* delColBtn = new QPushButton("×", header_);
  delColBtn->setFixedWidth(32);

  headerRow->addWidget(handle);
  headerRow->addWidget(title_, 1);
  headerRow->addWidget(addTaskBtn);
  headerRow->addWidget(delColBtn);

  root->addWidget(header_);

  list_ = new ColumnListWidget(columnId_, this);
  root->addWidget(list_, 1);

  connect(addTaskBtn, &QPushButton::clicked, this, [this]{ emit addTaskRequested(columnId_); });
  connect(delColBtn, &QPushButton::clicked, this, [this]{ emit deleteColumnRequested(columnId_); });
}

void KanbanColumnWidget::setTitle(const QString& t)
{
  if (title_)
    title_->setText(t);
}

void KanbanColumnWidget::startColumnDrag()
{
  auto* drag = new QDrag(this);

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_15);
  out << static_cast<qulonglong>(columnId_);
  out << static_cast<qulonglong>(boardId_);

  auto* md = new QMimeData();
  md->setData(QString::fromLatin1(kColumnMime), bytes);

  drag->setMimeData(md);
  drag->exec(Qt::MoveAction, Qt::MoveAction);
}

void KanbanColumnWidget::handleColumnDrop(unsigned long fromColumnId, bool insertBefore)
{
  if (fromColumnId == 0 || fromColumnId == columnId_)
    return;

  emit columnMoveRequested(fromColumnId, columnId_, insertBefore);
}