#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace core
{
  using Id = std::uint64_t;

  struct BoardDto
  {
    Id id{};
    std::string name;
    std::string description;
  };

  struct ColumnDto
  {
    Id id{};
    Id boardId{};
    std::string name;
    int sortOrder{};
  };

  struct TaskDto
  {
    Id id{};
    Id boardId{};
    Id columnId{};
    std::string title;
    std::string description;
    int sortOrder{};
    bool archived{};
  };

  class IKanbanApi
  {
  public:
    virtual ~IKanbanApi() = default;

    // Boards
    virtual std::vector<BoardDto> listBoards() = 0;
    virtual Id createBoard(const std::string& name, const std::string& description = {}) = 0;
    virtual void deleteBoard(Id boardId) = 0;

    // Columns
    virtual std::vector<ColumnDto> listColumns(Id boardId) = 0;          // ordered by sortOrder
    virtual Id createColumn(Id boardId, const std::string& name) = 0;    // append to end
    virtual void deleteColumn(Id columnId) = 0;                          // delete tasks inside, renumber columns

    // Tasks
    virtual std::vector<TaskDto> listTasksForBoard(Id boardId, bool includeArchived = false) = 0;
    virtual std::vector<TaskDto> listTasksForColumn(Id columnId, bool includeArchived = false) = 0;

    virtual Id createTask(Id boardId,
                          Id columnId,
                          const std::string& title,
                          const std::string& description = {}) = 0;

    virtual void deleteTask(Id taskId) = 0;

    // Ключевой метод для UI: UI говорит "вот новый порядок задач в колонке".
    // Это покрывает и reorder внутри колонки, и перенос между колонками (вызвать для обеих колонок).
    virtual void setTasksOrderForColumn(Id columnId, const std::vector<Id>& orderedTaskIds) = 0;
  };
} // namespace core 