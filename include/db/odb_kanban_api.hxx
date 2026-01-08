#pragma once

#include <memory>
#include "core/kanban_api.hxx"

namespace odb { namespace sqlite { class database; } }

namespace db
{
  class OdbKanbanApi final : public core::IKanbanApi
  {
  public:
    explicit OdbKanbanApi(std::shared_ptr<odb::sqlite::database> db);

    std::vector<core::BoardDto> listBoards() override;
    core::Id createBoard(const std::string& name, const std::string& description) override;
    void deleteBoard(core::Id boardId) override;

    std::vector<core::ColumnDto> listColumns(core::Id boardId) override;
    core::Id createColumn(core::Id boardId, const std::string& name) override;
    void deleteColumn(core::Id columnId) override;

    std::vector<core::TaskDto> listTasksForBoard(core::Id boardId, bool includeArchived) override;
    std::vector<core::TaskDto> listTasksForColumn(core::Id columnId, bool includeArchived) override;

    core::Id createTask(core::Id boardId,
                        core::Id columnId,
                        const std::string& title,
                        const std::string& description) override;

    void deleteTask(core::Id taskId) override;

    void setTasksOrderForColumn(core::Id columnId, const std::vector<core::Id>& orderedTaskIds) override;
    
    std::vector<core::CategoryDto> listCategories(core::Id boardId) override;
    core::Id createCategory(core::Id boardId, const std::string& name, const std::string& color) override;
    void deleteCategory(core::Id categoryId) override;
    void setTaskCategory(core::Id taskId, std::optional<core::Id> categoryId) override;
  private:
    std::shared_ptr<odb::sqlite::database> db_;
  };
} // namespace db