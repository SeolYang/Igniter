#include <Igniter.h>
#include <Component/NameComponent.h>
#include <Core/Engine.h>
#include <Core/Log.h>
#include <Core/ContainerUtils.h>
#include <Gameplay/GameInstance.h>
#include <ImGui/EntityList.h>

namespace ig
{
    void EntityList::Render()
    {
        if (ImGui::Begin("Entity List", &bIsVisible))
        {
            bool bIsItemClicked = false;
            if (ImGui::BeginChild("EntityListChild"))
            {
                if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& registry        = Igniter::GetGameInstance().GetRegistry();
                    auto  entityView      = std::views::all(registry.template storage<Entity>());
                    auto  validEntityView = std::views::filter([&registry](const Entity entity)
                    {
                        return registry.valid(entity);
                    });
                    auto entityNameView = std::views::transform(
                        [&registry](const Entity entity)
                        {
                            std::string formattedName{};

                            if (registry.all_of<NameComponent>(entity))
                            {
                                auto& nameComponent = registry.get<NameComponent>(entity);
                                formattedName       = std::format("{} ({})", nameComponent.Name.ToStringView(),
                                                                  entt::to_integral(entity));
                            }
                            else
                            {
                                formattedName = std::format("Entity {}", entt::to_integral(entity));
                            }

                            return std::make_pair(entity, formattedName);
                        });

                    std::vector<std::pair<Entity, std::string>> entityNames{
                        ToVector(entityView | validEntityView | entityNameView)
                    };
                    std::sort(entityNames.begin(), entityNames.end(),
                              [](const auto& lhs, const auto& rhs)
                              {
                                  return lhs.second < rhs.second;
                              });

                    constexpr ImGuiTreeNodeFlags TreeNodeBaseFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                            ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
                    for (const auto& [entity, entityName] : entityNames)
                    {
                        auto       nodeFlags   = TreeNodeBaseFlags;
                        const bool bIsSelected = entity == selectedEntity;
                        if (bIsSelected)
                        {
                            nodeFlags |= ImGuiTreeNodeFlags_Selected;
                        }

                        bool bIsOpened = ImGui::TreeNodeEx(entityName.c_str(), nodeFlags);
                        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                        {
                            selectedEntity = entity;
                            bIsItemClicked = true;
                        }

                        if (bIsOpened)
                        {
                            // if bIsOpened -> render childs ... -> recursively
                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }

                ImGui::EndChild();
            }

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0) && !bIsItemClicked)
            {
                selectedEntity = entt::null;
            }
            ImGui::End();
        }
    }
} // namespace ig
