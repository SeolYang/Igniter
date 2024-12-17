#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Meta.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/Gui/EntityInsepctor.h"
#include "Frieren/Gui/EntityList.h"

namespace fe
{
    EntityInspector::EntityInspector(const EntityList& entityList) : entityList(&entityList)
    {
        for (const auto& [typeID, type] : entt::resolve())
        {
            if (!ig::meta::IsComponent(type))
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

            componentInfos.emplace_back(ComponentInfo{ typeID, type, *name, ig::String{std::format("Detach Component##{}", *name)},
                ig::String{std::format("{}##SelectableComponent", *name)} });
        }

        componentInfos.shrink_to_fit();
        componentInfoIndicesToDisplay.reserve(componentInfos.size());
        addableComponentInfoIndices.reserve(componentInfos.size());
    }

    void EntityInspector::OnImGui()
    {
        IG_CHECK(entityList != nullptr);
        IG_CHECK(activeWorld != nullptr);
        const ig::Entity selectedEntity = entityList->GetSelectedEntity();
        UpdateDirty(selectedEntity);
        if (selectedEntity == entt::null)
        {
            ImGui::Text("Entity not selected.");
            return;
        }

        constexpr std::string_view attachComponentPopup{ "AttachComponentPopup" };
        ig::Registry& registry = activeWorld->GetRegistry();
        if (ImGui::Button("Attach Component", ImVec2(-FLT_MIN, 0.0f)))
        {
            ImGui::OpenPopup(attachComponentPopup.data());
        }

        if (ImGui::BeginPopup(attachComponentPopup.data()))
        {
            static ImGuiTextFilter filter;
            filter.Draw();
            ImGui::Separator();
            for (const size_t addableComponentInfoIdx : addableComponentInfoIndices)
            {
                const ComponentInfo& componentInfo = componentInfos[addableComponentInfoIdx];
                if (!filter.PassFilter(componentInfo.NameToDisplay.ToCString()))
                {
                    continue;
                }

                if (ImGui::Selectable(componentInfo.AttachSelectableLabel.ToCString()))
                {
                    if (ig::meta::Invoke(componentInfo.Type, ig::meta::AddComponentFunc, ig::Ref{ registry }, selectedEntity))
                    {
                        bForceDirty = true;
                    }
                }
            }
            ImGui::EndPopup();
        }

        const bool bDisplayableComponentDoesNotExists = componentInfoIndicesToDisplay.empty();
        if (bDisplayableComponentDoesNotExists)
        {
            ImGui::Text("Oops! No displayable components here!");
            return;
        }

        for (const size_t componentInfoIdx : componentInfoIndicesToDisplay)
        {
            const ComponentInfo& componentInfo = componentInfos[componentInfoIdx];
            if (!ImGui::CollapsingHeader(componentInfo.NameToDisplay.ToCString(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                continue;
            }

            if (ImGui::Button(componentInfo.RemoveButtonLabel.ToCString(), ImVec2(-FLT_MIN, 0.0f)))
            {
                componentToRemove = componentInfoIdx;
            }

            if (!ig::meta::Invoke(componentInfo.Type, ig::meta::OnInspectorFunc, &registry, selectedEntity))
            {
                continue;
            }
        }

        if (componentToRemove != (size_t)-1)
        {
            ImGui::OpenPopup("Detach Component?");
        }

        if (ImGui::BeginPopupModal("Detach Component?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("정말로 컴포넌트를 제거 하시겠습니까?");
            if (ImGui::Button("OK"))
            {
                const ComponentInfo& componentInfo = componentInfos[componentToRemove];
                if (ig::meta::Invoke(componentInfo.Type, ig::meta::RemoveComponentFunc, ig::Ref{ registry }, selectedEntity))
                {
                    // #sy_todo 컴포넌트 제거 사실 로깅?
                    bForceDirty = true;
                }

                componentToRemove = (size_t)-1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                componentToRemove = (size_t)-1;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void EntityInspector::UpdateDirty(const ig::Entity selectedEntity)
    {
        const bool bIsDirtry = selectedEntity != latestEntity || bForceDirty;
        if (!bIsDirtry)
        {
            return;
        }

        bForceDirty = false;
        componentInfoIndicesToDisplay.clear();
        addableComponentInfoIndices.clear();
        latestEntity = selectedEntity;
        if (selectedEntity == ig::NullEntity)
        {
            return;
        }

        ig::Registry& registry = activeWorld->GetRegistry();
        for (size_t idx = 0; idx < componentInfos.size(); ++idx)
        {
            const ComponentInfo& componentInfo = componentInfos[idx];
            const entt::sparse_set* componentStorage = registry.storage(componentInfo.ID);
            if (componentStorage == nullptr || !componentStorage->contains(selectedEntity))
            {
                addableComponentInfoIndices.emplace_back(idx);
            }
            else
            {
                componentInfoIndicesToDisplay.emplace_back(idx);
            }
        }
    }
}    // namespace fe
