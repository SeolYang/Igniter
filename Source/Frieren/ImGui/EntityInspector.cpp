#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Meta.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/ImGui/EntityInsepctor.h"
#include "Frieren/ImGui/EntityList.h"

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

            const bool bIsRemovableComponent = entt::resolve<ig::NameComponent>() != type && entt::resolve<ig::TransformComponent>() != type;
            componentInfos.emplace_back(ComponentInfo{typeID, type, *name, ig::String{std::format("Remove##{}", *name)}, bIsRemovableComponent});
        }

        componentInfos.shrink_to_fit();
        componentInfoIndicesToDisplay.reserve(componentInfos.size());
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

        const bool bDisplayableComponentDoesNotExists = componentInfoIndicesToDisplay.empty();
        if (bDisplayableComponentDoesNotExists)
        {
            ImGui::Text("Oops! No displayable components here!");
            return;
        }

        ig::Registry& registry = activeWorld->GetRegistry();
        for (const size_t componentInfoIdx : componentInfoIndicesToDisplay)
        {
            const ComponentInfo& componentInfo = componentInfos[componentInfoIdx];
            if (!ImGui::CollapsingHeader(componentInfo.NameToDisplay.ToCString(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                continue;
            }

            if (!ig::meta::Invoke(componentInfo.Type, ig::meta::OnInspectorFunc, &registry, selectedEntity))
            {
                continue;
            }

            if (componentInfo.bIsRemovable && ImGui::Button(componentInfo.RemoveButtonLabel.ToCString()))
            {
                if (ig::meta::Invoke(componentInfo.Type, ig::meta::RemoveComponentFunc, ig::Ref{registry}, selectedEntity))
                {
                    // #sy_todo 컴포넌트 제거 사실 로깅?
                    bForceDirty = true;
                }
            }
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
                continue;
            }

            componentInfoIndicesToDisplay.emplace_back(idx);
        }
    }
}    // namespace fe
