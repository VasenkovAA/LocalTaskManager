#include "gui/task_edit_dialog.hxx"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include <odb/sqlite/database.hxx>
#include <odb/transaction.hxx>

#include "models/task.hxx"
#include "models/task_category.hxx"

#include "task-odb.hxx"
#include "task_category-odb.hxx"

// ...

TaskEditDialog::TaskEditDialog(std::shared_ptr<odb::sqlite::database> db,
                               unsigned long taskId, QWidget *parent)
    : QDialog(parent), db_(std::move(db)), taskId_(taskId) {
  setWindowTitle("Edit task");
  resize(520, 420);

  auto *root = new QVBoxLayout(this);

  auto *form = new QFormLayout();

  title_ = new QLineEdit(this);
  desc_ = new QTextEdit(this);

  // Category row: combobox + Add button
  auto *catRow = new QWidget(this);
  auto *catLayout = new QHBoxLayout(catRow);
  catLayout->setContentsMargins(0, 0, 0, 0);
  catLayout->setSpacing(8);

  category_ = new QComboBox(catRow);
  addCategoryBtn_ = new QPushButton("Add...", catRow);
  addCategoryBtn_->setEnabled(false); // включим после load()
  catLayout->addWidget(category_, 1);
  catLayout->addWidget(addCategoryBtn_);

  form->addRow("Title:", title_);
  form->addRow("Category:", catRow);
  form->addRow("Description:", desc_);

  root->addLayout(form);

  connect(addCategoryBtn_, &QPushButton::clicked, this,
          [this] { onAddCategory(); });

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &TaskEditDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &TaskEditDialog::reject);
  root->addWidget(buttons);

  load();
}
std::optional<TaskEditDialog::TaskSnapshot>
TaskEditDialog::editTask(std::shared_ptr<odb::sqlite::database> db,
                         unsigned long taskId, QWidget *parent) {
  if (!db || taskId == 0)
    return std::nullopt;

  TaskEditDialog dlg(std::move(db), taskId, parent);
  if (dlg.exec() != QDialog::Accepted)
    return std::nullopt;

  return dlg.snapshot_;
}
void TaskEditDialog::load() {
  try {
    odb::transaction t(db_->begin());

    std::unique_ptr<Task> task(db_->load<Task>(taskId_));
    if (!task)
      throw std::runtime_error("Task not found");

    boardId_ = task->board_id();

    // ВАЖНО: используем вариант БЕЗ транзакции (мы уже внутри t)
    reloadCategoriesNoTx(boardId_);

    title_->setText(QString::fromStdString(task->title()));
    desc_->setPlainText(QString::fromStdString(task->description()));

    const auto cat = task->category_id();
    const qulonglong catId =
        cat.null() ? 0 : static_cast<qulonglong>(cat.get());

    int idx = category_->findData(QVariant::fromValue<qulonglong>(catId));
    if (idx < 0)
      idx = 0;
    category_->setCurrentIndex(idx);

    t.commit();

    if (addCategoryBtn_)
      addCategoryBtn_->setEnabled(true);
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Database Error",
                          QString("Failed to load task: %1").arg(e.what()));
    reject();
  }
}

void TaskEditDialog::accept() {
  const QString newTitle = title_->text().trimmed();
  if (newTitle.isEmpty()) {
    QMessageBox::warning(this, "Validation", "Title cannot be empty.");
    return;
  }

  const QString newDesc = desc_->toPlainText();

  const qulonglong rawCatId = category_->currentData().toULongLong();
  const std::optional<unsigned long> newCatId =
      (rawCatId == 0)
          ? std::nullopt
          : std::optional<unsigned long>(static_cast<unsigned long>(rawCatId));

  try {
    odb::transaction t(db_->begin());

    std::unique_ptr<Task> task(db_->load<Task>(taskId_));
    if (!task)
      throw std::runtime_error("Task not found");

    task->title(newTitle.toStdString());
    task->description(newDesc.toStdString());

    if (!newCatId.has_value())
      task->clear_category();
    else
      task->category_id(*newCatId);

    db_->update(*task);

    snapshot_.taskId = task->id();
    snapshot_.columnId = task->column_id();
    snapshot_.title = newTitle;
    snapshot_.description = newDesc;
    snapshot_.categoryId = newCatId;

    t.commit();
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Database Error",
                          QString("Failed to save task: %1").arg(e.what()));
    return;
  }

  QDialog::accept();
}
void TaskEditDialog::reloadCategoriesNoTx(unsigned long boardId) {
  category_->clear();
  category_->addItem("None", QVariant::fromValue<qulonglong>(0));

  using CQ = odb::query<TaskCategory>;
  std::vector<TaskCategory> cats;
  for (const auto &c : db_->query<TaskCategory>(CQ::board_id == boardId))
    cats.push_back(c);

  std::sort(cats.begin(), cats.end(),
            [](const TaskCategory &a, const TaskCategory &b) {
              if (a.sort_order() != b.sort_order())
                return a.sort_order() < b.sort_order();
              return a.id() < b.id();
            });

  for (const auto &c : cats)
    category_->addItem(QString::fromStdString(c.name()),
                       QVariant::fromValue<qulonglong>(c.id()));
}

void TaskEditDialog::reloadCategoriesTx(unsigned long boardId) {
  odb::transaction t(db_->begin());
  reloadCategoriesNoTx(boardId);
  t.commit();
}
void TaskEditDialog::onAddCategory() {
  if (!db_ || boardId_ == 0)
    return;

  bool ok = false;
  QString name = QInputDialog::getText(this, "Add category",
                                       "Name:", QLineEdit::Normal, "", &ok);
  if (!ok)
    return;
  name = name.trimmed();
  if (name.isEmpty())
    return;

  QColor chosen =
      QColorDialog::getColor(Qt::white, this, "Choose color (optional)");
  const QString colorStr = chosen.isValid() ? chosen.name(QColor::HexRgb)
                                            : QString(); // "" => no color

  unsigned long newCatId = 0;

  try {
    odb::transaction t(db_->begin());

    int maxOrder = -1;
    using CQ = odb::query<TaskCategory>;
    for (const auto &c : db_->query<TaskCategory>(CQ::board_id == boardId_))
      maxOrder = std::max(maxOrder, c.sort_order());

    TaskCategory cat(boardId_, name.toStdString(), maxOrder + 1,
                     colorStr.toStdString());
    db_->persist(cat);
    newCatId = cat.id();

    t.commit();
  } catch (const std::exception &e) {
    QMessageBox::critical(
        this, "Database Error",
        QString("Failed to create category: %1").arg(e.what()));
    return;
  }

  // Перезагрузим список категорий (с отдельной транзакцией)
  reloadCategoriesTx(boardId_);

  // Выберем новую категорию
  int idx = category_->findData(QVariant::fromValue<qulonglong>(newCatId));
  if (idx >= 0)
    category_->setCurrentIndex(idx);
}