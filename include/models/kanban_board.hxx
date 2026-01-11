#ifndef KANBAN_BOARD_HXX
#define KANBAN_BOARD_HXX

#include <odb/core.hxx>
#include <string>
#include <utility>

#pragma db object
class KanbanBoard {
public:
  KanbanBoard() = default;

  KanbanBoard(std::string name, std::string description = {})
      : name_(std::move(name)), description_(std::move(description)) {}

  unsigned long id() const { return id_; }

  const std::string &name() const { return name_; }
  void name(std::string v) { name_ = std::move(v); }

  const std::string &description() const { return description_; }
  void description(std::string v) { description_ = std::move(v); }

private:
  friend class odb::access;

#pragma db id auto
  unsigned long id_{0};

  std::string name_;
  std::string description_;
};

#endif // KANBAN_BOARD_HXX