#ifndef LTM_TASK_HXX
#define LTM_TASK_HXX

#include <string>
#include <odb/core.hxx>

// Простая модель задачи
#pragma db object
class task
{
public:
  task() = default; // обязателен для ODB

  task(const std::string& title,
       const std::string& description,
       bool completed = false)
    : title_(title),
      description_(description),
      completed_(completed)
  {
  }

  // Геттеры
  unsigned long id() const               { return id_; }
  const std::string& title() const       { return title_; }
  const std::string& description() const { return description_; }
  bool completed() const                 { return completed_; }

  // Сеттеры
  void set_title(const std::string& t)        { title_ = t; }
  void set_description(const std::string& d)  { description_ = d; }
  void set_completed(bool c)                  { completed_ = c; }

private:
  friend class odb::access; // даём ODB доступ к приватным полям

  #pragma db id auto
  unsigned long id_; // автоинкрементный первичный ключ

  std::string title_;
  std::string description_;
  bool completed_;
};

#endif // LTM_TASK_HXX