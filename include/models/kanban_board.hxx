#ifndef KANBAN_BOARD_HXX
#define KANBAN_BOARD_HXX

#include <string>
#include <odb/core.hxx>

#pragma db object
class KanbanBoard
{
public:
  KanbanBoard() = default;
  KanbanBoard(const std::string& name, const std::string& description = {})
      : name_(name), description_(description) {}

  unsigned long id() const { return id_; }

  const std::string& name() const { return name_; }
  void name(const std::string& v) { name_ = v; }

  const std::string& description() const { return description_; }
  void description(const std::string& v) { description_ = v; }

private:
  friend class odb::access;

#pragma db id auto
  unsigned long id_;

  std::string name_;
  std::string description_;
};

#endif //KANBAN_BOARD_HXX
