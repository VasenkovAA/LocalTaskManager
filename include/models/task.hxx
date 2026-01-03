#ifndef TASK_HPP
#define TASK_HPP

#include <string>

#include <odb/core.hxx>

#pragma db object
class Task {
public:
  Task(std::string &title, std::string &description)
      : title_(title), description_(description) {}
  Task(std::string title, std::string description)
      : title_(title), description_(description) {}
  Task() {}
  std::string title() const { return title_; }
  std::string description() const { return description_; }

private:
#pragma db id auto
  unsigned long id_;
  std::string title_;
  std::string description_;

  friend class odb::access;
};

#endif // TASK_HPP