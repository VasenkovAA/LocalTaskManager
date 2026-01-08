#ifndef TASK_HXX
#define TASK_HXX

#include <string>
#include <utility>
#include <odb/core.hxx>
#include <odb/nullable.hxx>

#pragma db object
#pragma db index member(board_id_)
#pragma db index member(column_id_)
#pragma db index member(category_id_)
class Task
{
  #pragma db index("idx_task_col_sort") members(column_id_, sort_order_)
public:
  Task() = default;

  Task(unsigned long board_id,
       unsigned long column_id,
       std::string title,
       std::string description = {},
       int sort_order = 0)
      : board_id_(board_id),
        column_id_(column_id),
        title_(std::move(title)),
        description_(std::move(description)),
        sort_order_(sort_order)
  {}

  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  unsigned long column_id() const { return column_id_; }
  void column_id(unsigned long v) { column_id_ = v; }

  odb::nullable<unsigned long> category_id() const { return category_id_; }
  void category_id(const odb::nullable<unsigned long>& v) { category_id_ = v; }
  void category_id(unsigned long v) { category_id_ = v; }
  void clear_category() { category_id_.reset(); }
  bool has_category() const { return !category_id_.null(); }

  const std::string& title() const { return title_; }
  void title(std::string v) { title_ = std::move(v); }

  const std::string& description() const { return description_; }
  void description(std::string v) { description_ = std::move(v); }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

  bool archived() const { return archived_; }
  void archived(bool v) { archived_ = v; }

private:
  friend class odb::access;

  #pragma db id auto
  unsigned long id_{0};

  unsigned long board_id_{0};
  unsigned long column_id_{0};
  odb::nullable<unsigned long> category_id_;

  std::string title_;
  std::string description_;

  int sort_order_{0};
  bool archived_{false};
};

#endif // TASK_HXX