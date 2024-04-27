#include <Frieren.h>
#include <Core/Engine.h>
#include <Core/Meta.h>
#include <Gameplay/GameInstance.h>
#include <Component/NameComponent.h>
#include <ImGui/EntityInsepctor.h>
#include <ImGui/EntityList.h>

namespace fe
{
    EntityInspector::EntityInspector(const EntityList& entityList) : entityList(&entityList) {}

    void EntityInspector::Render()
    {
        IG_CHECK(entityList != nullptr);
        if (!ImGui::Begin("Inspector", &bIsVisible))
        {
            return;
        }

        const Entity selectedEntity = entityList->GetSelectedEntity();
        if (selectedEntity == entt::null)
        {
            ImGui::Text("Entity not selected.");
            ImGui::End();
            return;
        }

        GameInstance& gameInstance = Igniter::GetGameInstance();
        Registry& registry = gameInstance.GetRegistry();
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

            entt::meta_prop nameProperty = type.prop(ig::meta::NameProperty);
            if (!nameProperty)
            {
                continue;
            }

            const String* name = nameProperty.value().try_cast<String>();
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
        ImGui::End();
    }
}    // namespace fe
