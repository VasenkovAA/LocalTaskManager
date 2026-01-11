#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace core {
using Id = std::uint64_t;

struct BoardDto {
  Id id{};
  std::string name;
  std::string description;
};

struct ColumnDto {
  Id id{};
  Id boardId{};
  std::string name;
  int sortOrder{};
};

struct TaskDto {
  Id id{};
  Id boardId{};
  Id columnId{};
  std::optional<Id> categoryId;
  std::string title;
  std::string description;
  int sortOrder{};
  bool archived{};
};

struct CategoryDto {
  Id id{};
  Id boardId{};
  std::string name;
  int sortOrder{};
  std::string color;
};

class IKanbanApi {
public:
  virtual ~IKanbanApi() = default;

  // Boards
  virtual std::vector<BoardDto> listBoards() = 0;
  virtual Id createBoard(const std::string &name,
                         const std::string &description = {}) = 0;
  virtual void deleteBoard(Id boardId) = 0;

  // Columns
  virtual std::vector<ColumnDto>
  listColumns(Id boardId) = 0; // ordered by sortOrder
  virtual Id createColumn(Id boardId,
                          const std::string &name) = 0; // append to end
  virtual void
  deleteColumn(Id columnId) = 0; // delete tasks inside, renumber columns

  // Tasks
  virtual std::vector<TaskDto>
  listTasksForBoard(Id boardId, bool includeArchived = false) = 0;
  virtual std::vector<TaskDto>
  listTasksForColumn(Id columnId, bool includeArchived = false) = 0;

  virtual Id createTask(Id boardId, Id columnId, const std::string &title,
                        const std::string &description = {}) = 0;

  virtual void deleteTask(Id taskId) = 0;

  // Categories
  virtual std::vector<CategoryDto> listCategories(Id boardId) = 0;
  virtual Id createCategory(Id boardId, const std::string &name,
                            const std::string &color = {}) = 0;
  virtual void deleteCategory(Id categoryId) = 0;

  // Task <-> Category
  virtual void setTaskCategory(Id taskId, std::optional<Id> categoryId) = 0;

  virtual void
  setTasksOrderForColumn(Id columnId,
                         const std::vector<Id> &orderedTaskIds) = 0;
};
} // namespace core