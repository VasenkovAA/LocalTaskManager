#ifndef KANBAN_COLUMN_HXX
#define KANBAN_COLUMN_HXX

#include <string>
#include <odb/core.hxx>

#pragma db object
#pragma db index member(board_id_)
class KanbanColumn
{
public:
  KanbanColumn() : board_id_(0), sort_order_(0) {}

  KanbanColumn(unsigned long board_id, const std::string& name, int sort_order = 0)
      : board_id_(board_id), name_(name), sort_order_(sort_order) {}

  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  const std::string& name() const { return name_; }
  void name(const std::string& v) { name_ = v; }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

private:
  friend class odb::access;

#pragma db id auto
  unsigned long id_;

  unsigned long board_id_;
  std::string name_;
  int sort_order_;
};

#endif //KANBAN_BOARD_HXX
