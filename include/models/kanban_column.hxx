#ifndef KANBAN_COLUMN_HXX
#define KANBAN_COLUMN_HXX

#include <odb/core.hxx>
#include <string>
#include <utility>

#pragma db object
#pragma db index member(board_id_)
class KanbanColumn {
#pragma db index("idx_column_board_sort") members(board_id_, sort_order_)
public:
  KanbanColumn() = default;

  KanbanColumn(unsigned long board_id, std::string name, int sort_order = 0)
      : board_id_(board_id), name_(std::move(name)), sort_order_(sort_order) {}

  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  const std::string &name() const { return name_; }
  void name(std::string v) { name_ = std::move(v); }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

private:
  friend class odb::access;

#pragma db id auto
  unsigned long id_{0};

  unsigned long board_id_{0};
  std::string name_;
  int sort_order_{0};
};

#endif // KANBAN_COLUMN_HXX