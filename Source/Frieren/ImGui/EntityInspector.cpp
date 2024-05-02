#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Meta.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Frieren/ImGui/EntityInsepctor.h"
#include "Frieren/ImGui/EntityList.h"

namespace fe
{
    EntityInspector::EntityInspector(const EntityList& entityList) : entityList(&entityList) {}

    void EntityInspector::OnImGui()
    {
        IG_CHECK(entityList != nullptr);
        const ig::Entity selectedEntity = entityList->GetSelectedEntity();
        if (selectedEntity == entt::null)
        {
            ImGui::Text("Entity not selected.");
            return;
        }

        if (activeWorld == nullptr)
        {
            return;
        }

        ig::Registry& registry = activeWorld->GetRegistry();
        bool bHasDisplayableComponent = false;

        for (const auto& [typeID, type] : entt::resolve())
        {
            if (!type.prop(ig::meta::ComponentProperty))
            {
                continue;
            }

            entt::sparse_set* componentStorage = registry.storage(typeID);
            if (componentStorage == nullptr || !componentStorage->contains(selectedEntity))
            {
                continue;
            }

            entt::meta_prop nameProperty = type.prop(ig::meta::TitleCaseNameProperty);
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
            if (name == nullptr)
            {
                continue;
            }

            bHasDisplayableComponent = true;

            if (!ImGui::CollapsingHeader(name->ToCString(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                continue;
            }

            entt::meta_func onEditorFunc = type.func(ig::meta::OnInspectorFunc);
            if (!onEditorFunc)
            {
                continue;
            }
            onEditorFunc.invoke(type, &registry, selectedEntity);
        }

        if (!bHasDisplayableComponent)
        {
            ImGui::Text("Oops! No Displayable Components here!");
        }
    }
}    // namespace fe
