#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Frieren/Gui/EntityList.h"

IG_DECLARE_LOG_CATEGORY(EditorEntityList);
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
                constexpr std::string_view EntityManagementPopupID{"EntityManagementPopup"};
                constexpr ImGuiTreeNodeFlags TreeNodeBaseFlags =
                    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                // #sy_todo 좀 더 최적화 필요, Name 기반 sorting 필요..
                const auto entityView = registry.view<ig::Entity>();
                entites.clear();
                entites.reserve(entityView.size_hint());
                for (const ig::Entity entity : registry.view<ig::Entity>())
                {
                    if (registry.valid(entity))
                    {
                        entites.emplace_back(entity);
                    }
                }

                bool bShouldOpenEntityManagementPopup = false;
                ImGuiListClipper clipper;
                clipper.Begin((ig::S32)entites.size());
                while (clipper.Step())
                {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                    {
                        const ig::Entity entity = entites[row];
                        const auto* nameComponent = registry.try_get<ig::NameComponent>(entity);
                        std::string entityName;
                        if (nameComponent != nullptr)
                        {
                            entityName = std::format("{} ({})", nameComponent->Name, entt::to_integral(entity));
                        }
                        else
                        {
                            entityName = std::format("Unnamed ({})", entt::to_integral(entity));
                        }

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

            constexpr std::string_view EntityCreatePopupID{"EntityCreatePopup"};
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !bIsItemClicked)
            {
                ImGui::OpenPopup(EntityCreatePopupID.data());
            }

            if (ImGui::BeginPopup(EntityCreatePopupID.data()))
            {
                if (ImGui::Selectable("Create Empty Entity"))
                {
                    const ig::Entity newEntity = registry.create();
                    selectedEntity = newEntity;
                }

                for (const auto& [typeId, type] : entt::resolve())
                {
                    if (!ig::meta::IsArchetype(type))
                    {
                        continue;
                    }

                    ig::meta::Property nameProperty = type.prop(ig::meta::TitleCaseNameProperty);
                    if (!nameProperty)
                    {
                        // fallback
                        nameProperty = type.prop(ig::meta::NameProperty);
                        if (!nameProperty)
                        {
                            continue;
                        }
                    }

                    const ig::String* name = nameProperty.value().try_cast<ig::String>();
                    IG_CHECK(name != nullptr);

                    if (ImGui::Selectable(std::format("Create {}", *name).c_str()))
                    {
                        const ig::meta::Function function = type.func(ig::meta::ArchetypeCreateFunc);
                        selectedEntity = function.invoke(type, &registry).cast<ig::Entity>();
                    }
                }

                ImGui::EndPopup();
            }
        }
    }
} // namespace fe
