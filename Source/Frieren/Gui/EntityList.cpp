#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Frieren/Gui/EntityList.h"

IG_DEFINE_LOG_CATEGORY(EditorEntityList);

namespace fe
{
    void EntityList::OnImGui()
    {
        bool bIsItemClicked = false;
        if (activeWorld != nullptr)
        {
            auto& registry = activeWorld->GetRegistry();
            if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto entityView = ig::views::all(registry.template storage<ig::Entity>());
                auto validEntityView = ig::views::filter([&registry](const ig::Entity entity) { return registry.valid(entity); });
                auto entityNameView = ig::views::transform([&registry](const ig::Entity entity)
                {
                    std::string formattedName{};

                    if (registry.all_of<ig::NameComponent>(entity))
                    {
                        auto& nameComponent = registry.get<ig::NameComponent>(entity);
                        formattedName = std::format("{} ({})", nameComponent.Name.ToStringView(), entt::to_integral(entity));
                    }
                    else
                    {
                        formattedName = std::format("Unnamed ({})", entt::to_integral(entity));
                    }

                    return std::make_pair(entity, formattedName);
                });

                eastl::vector<std::pair<ig::Entity, std::string>> entityNames{ ig::ToVector(entityView | validEntityView | entityNameView) };
                std::sort(entityNames.begin(), entityNames.end(), [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

                constexpr std::string_view EntityManagementPopupID{ "EntityManagementPopup" };
                constexpr ImGuiTreeNodeFlags TreeNodeBaseFlags =
                    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                bool bShouldOpenEntityManagementPopup = false;
                for (const auto& [entity, entityName] : entityNames)
                {
                    auto nodeFlags = TreeNodeBaseFlags;
                    const bool bIsSelected = entity == selectedEntity;
                    if (bIsSelected)
                    {
                        nodeFlags |= ImGuiTreeNodeFlags_Selected;
                    }

                    const bool bIsOpened = ImGui::TreeNodeEx(entityName.c_str(), nodeFlags);
                    const bool bIsLeftMouseButtonClicked = ImGui::IsItemClicked();
                    const bool bIsRightMouseButtonClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
                    const bool bIsAnyMouseClicked = bIsLeftMouseButtonClicked || bIsRightMouseButtonClicked;
                    const bool bIsClickedByMouse = ImGui::IsItemHovered() && bIsAnyMouseClicked;
                    const bool bIsClickedByKeyboard = ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemToggledOpen();
                    if (const bool bShouldSelectEntity = bIsClickedByMouse || bIsClickedByKeyboard; bShouldSelectEntity)
                    {
                        selectedEntity = entity;
                        bIsItemClicked = true;

                        if (bIsRightMouseButtonClicked)
                        {
                            bShouldOpenEntityManagementPopup = true;
                        }
                    }

                    if (bIsOpened)
                    {
                        // if bIsOpened -> render childs ... -> recursively
                        ImGui::TreePop();
                    }
                }

                if (bShouldOpenEntityManagementPopup)
                {
                    ImGui::OpenPopup(EntityManagementPopupID.data());
                }

                if (ImGui::BeginPopup(EntityManagementPopupID.data()))
                {
                    ImGui::SeparatorText("Manage Entity");
                    if (ImGui::Selectable("Delete"))
                    {
                        IG_CHECK(selectedEntity != ig::NullEntity);
                        const auto* nameComponent = registry.try_get<ig::NameComponent>(selectedEntity);
                        const ig::String nameStr = nameComponent == nullptr ? "Unnamed"_fs : nameComponent->Name;
                        IG_LOG(
                            EditorEntityList, Info, "Delete entity \"{} ({})\" from world.", nameStr, entt::to_integral(selectedEntity));
                        registry.destroy(selectedEntity);
                        selectedEntity = ig::NullEntity;
                    }

                    if (ImGui::Selectable("Clone(Not Impl Yet)", false, ImGuiSelectableFlags_Disabled))
                    {
                    }

                    ImGui::EndPopup();
                }

                ImGui::TreePop();
            }

            if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && (selectedEntity != ig::NullEntity))
            {
                registry.destroy(selectedEntity);
                selectedEntity = entt::null;
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !bIsItemClicked)
            {
                selectedEntity = entt::null;
            }

            constexpr std::string_view EntityCreatePopupID{ "EntityCreatePopup" };
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !bIsItemClicked)
            {
                ImGui::OpenPopup(EntityCreatePopupID.data());
            }

            if (ImGui::BeginPopup(EntityCreatePopupID.data()))
            {
                if (ImGui::Selectable("Create New Entity"))
                {
                    const ig::Entity newEntity = registry.create();
                    selectedEntity = newEntity;
                }
                ImGui::EndPopup();
            }
        }
    }
}    // namespace fe
