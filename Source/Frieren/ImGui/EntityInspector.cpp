#include <Frieren.h>
#include <Core/Engine.h>
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
        if (ImGui::Begin("Inspector", &bIsVisible))
        {
            const Entity selectedEntity = entityList->GetSelectedEntity();
            if (selectedEntity != entt::null)
            {
                GameInstance& gameInstance = Igniter::GetGameInstance();
                Registry& registry = gameInstance.GetRegistry();
                bool bHasDisplayableComponent = false;

                const auto componentInfos = ComponentRegistry::GetComponentInfos();
                for (const auto& componentInfoPair : componentInfos)
                {
                    const auto& [componentID, componentInfo] = componentInfoPair;

                    entt::sparse_set* sparseSet = registry.storage(componentID);
                    if (sparseSet != nullptr && sparseSet->contains(selectedEntity))
                    {
                        if (ImGui::CollapsingHeader(componentInfo.Name.data(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
                        {
                            componentInfo.OnImGui(registry, selectedEntity);
                        }
                        bHasDisplayableComponent = true;
                    }
                }

                if (!bHasDisplayableComponent)
                {
                    ImGui::Text("Oops! No Displayable Components here!");
                }
            }
            else
            {
                ImGui::Text("Entity not selected.");
            }

            ImGui::End();
        }
    }
}    // namespace fe
