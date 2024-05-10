#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/ImGui/EntityList.h"

namespace fe
{
    void EntityList::OnImGui()
    {
        bool bIsItemClicked = false;
        if (ImGui::BeginChild("EntityListChild"))
        {
            if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (activeWorld != nullptr)
                {
                    auto& registry = activeWorld->GetRegistry();
                    auto entityView = ig::views::all(registry.template storage<ig::Entity>());
                    auto validEntityView = ig::views::filter([&registry](const ig::Entity entity) { return registry.valid(entity); });
                    auto entityNameView = ig::views::transform(
                        [&registry](const ig::Entity entity)
                        {
                            std::string formattedName{};

                            if (registry.all_of<ig::NameComponent>(entity))
                            {
                                auto& nameComponent = registry.get<ig::NameComponent>(entity);
                                formattedName = std::format("{} ({})", nameComponent.Name.ToStringView(), entt::to_integral(entity));
                            }
                            else
                            {
                                formattedName = std::format("Entity {}", entt::to_integral(entity));
                            }

                            return std::make_pair(entity, formattedName);
                        });

                    eastl::vector<std::pair<ig::Entity, std::string>> entityNames{ig::ToVector(entityView | validEntityView | entityNameView)};
                    std::sort(entityNames.begin(), entityNames.end(), [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

                    constexpr ImGuiTreeNodeFlags TreeNodeBaseFlags =
                        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
                    for (const auto& [entity, entityName] : entityNames)
                    {
                        auto nodeFlags = TreeNodeBaseFlags;
                        const bool bIsSelected = entity == selectedEntity;
                        if (bIsSelected)
                        {
                            nodeFlags |= ImGuiTreeNodeFlags_Selected;
                        }

                        bool bIsOpened = ImGui::TreeNodeEx(entityName.c_str(), nodeFlags);
                        if ((ImGui::IsItemClicked() || ImGui::IsItemFocused()) && !ImGui::IsItemToggledOpen())
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
                }

                ImGui::TreePop();
            }

            ImGui::EndChild();
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0) && !bIsItemClicked)
        {
            selectedEntity = entt::null;
        }
    }
}    // namespace fe
