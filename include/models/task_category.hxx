#ifndef TASK_CATEGORY_HXX
#define TASK_CATEGORY_HXX

#include <odb/core.hxx>
#include <string>

#pragma db object
class TaskCategory {
public:
  TaskCategory() : board_id_(0), sort_order_(0) {}
  TaskCategory(unsigned long board_id, const std::string &name,
               int sort_order_ = 0)
      : board_id_(board_id), name_(name) {}
  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  const std::string &name() const { return name_; }
  void name(const std::string &v) { name_ = v; }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

  const std::string &color() const { return color_; }
  void color(const std::string &color) { color_ = color; }

private:
  friend class odb ::access;

  #pragma db id auto
  unsigned long id_;

  unsigned long board_id_;
  std::string name_;
  int sort_order_;
  std::string color_;
};

#endif // TASK_CATEGORY_HXX
