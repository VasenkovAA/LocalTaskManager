#ifndef TASK_HXX
#define TASK_HXX

#include <string>
#include <odb/core.hxx>

#pragma db object
#pragma db index member(board_id_)
#pragma db index member(column_id_)
class Task
{
public:
  Task()
      : board_id_(0), column_id_(0), sort_order_(0), archived_(false) {}

  Task(unsigned long board_id,
       unsigned long column_id,
       const std::string& title,
       const std::string& description = {},
       int sort_order = 0)
      : board_id_(board_id),
        column_id_(column_id),
        title_(title),
        description_(description),
        sort_order_(sort_order),
        archived_(false) {}

  unsigned long id() const { return id_; }

  unsigned long board_id() const { return board_id_; }
  void board_id(unsigned long v) { board_id_ = v; }

  unsigned long column_id() const { return column_id_; }
  void column_id(unsigned long v) { column_id_ = v; }

  const std::string& title() const { return title_; }
  void title(const std::string& v) { title_ = v; }

  const std::string& description() const { return description_; }
  void description(const std::string& v) { description_ = v; }

  int sort_order() const { return sort_order_; }
  void sort_order(int v) { sort_order_ = v; }

  bool archived() const { return archived_; }
  void archived(bool v) { archived_ = v; }

private:
  friend class odb::access;

#pragma db id auto
  unsigned long id_;

  unsigned long board_id_;
  unsigned long column_id_;

  std::string title_;
  std::string description_;

  int sort_order_;
  bool archived_;
};

#endif //TASK_HXX
