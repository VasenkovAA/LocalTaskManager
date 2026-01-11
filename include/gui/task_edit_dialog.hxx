#pragma once

#include <QDialog>
#include <memory>
#include <optional>

namespace odb {
namespace sqlite {
class database;
}
} // namespace odb

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton; // <-- добавить

class TaskEditDialog final : public QDialog {
  Q_OBJECT
public:
  struct TaskSnapshot {
    unsigned long taskId = 0;
    unsigned long columnId = 0;
    QString title;
    QString description;
    std::optional<unsigned long> categoryId;
  };

  static std::optional<TaskSnapshot>
  editTask(std::shared_ptr<odb::sqlite::database> db, unsigned long taskId,
           QWidget *parent = nullptr);

private:
  explicit TaskEditDialog(std::shared_ptr<odb::sqlite::database> db,
                          unsigned long taskId, QWidget *parent = nullptr);

  void load();

  // Новый подход:
  //  - reloadCategoriesNoTx() вызывается ТОЛЬКО когда транзакция уже открыта
  //  снаружи
  //  - reloadCategoriesTx() сам открывает транзакцию (для кнопки "Add
  //  category")
  void reloadCategoriesNoTx(unsigned long boardId);
  void reloadCategoriesTx(unsigned long boardId);

  void accept() override;

  void onAddCategory(); // <-- добавить

private:
  std::shared_ptr<odb::sqlite::database> db_;
  unsigned long taskId_ = 0;
  unsigned long boardId_ = 0;

  QLineEdit *title_ = nullptr;
  QTextEdit *desc_ = nullptr;
  QComboBox *category_ = nullptr;
  QPushButton *addCategoryBtn_ = nullptr; // <-- добавить

  TaskSnapshot snapshot_;
};