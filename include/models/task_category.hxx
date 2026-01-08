#ifndef TASK_CATEGORY_HXX
#define TASK_CATEGORY_HXX

#include <string>
#include <utility>
#include <odb/core.hxx>

#pragma db object
#pragma db index member(board_id_)

class TaskCategory
{
  #pragma db index("idx_cat_board_sort") members(board_id_, sort_order_)
public:
  TaskCategory() = default;

  TaskCategory(unsigned long board_id, std::string name, int sort_order = 0, std::string color = {})
      : board_id_(board_id),
        name_(std::move(name)),
        sort_order_(sort_order),
        color_(std::move(color))
  {}

  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  const std::string& name() const { return name_; }
  void name(std::string v) { name_ = std::move(v); }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

  const std::string& color() const { return color_; }
  void color(std::string v) { color_ = std::move(v); }

private:
  friend class odb::access;

  #pragma db id auto
  unsigned long id_{0};

  unsigned long board_id_{0};
  std::string name_;
  int sort_order_{0};
  std::string color_; // пусто => “без цвета”
};

#endif // TASK_CATEGORY_HXX