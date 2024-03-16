#include <ImGui/EntityList.h>
#include <ImGui/Common.h>
#include <Gameplay/Common.h>
#include <Gameplay/GameInstance.h>
#include <Core/NameComponent.h>
#include <Core/Igniter.h>

namespace ig
{
    EntityList::EntityList()
        : ImGuiLayer("Entity List"_fs)
    {
    }

    void EntityList::Render()
    {
        if (ImGui::Begin("List of Entity", &bIsVisible))
        {
            if (ImGui::TreeNodeEx("Entities", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& registry = Igniter::GetGameInstance().GetRegistry();
                auto entityView = std::views::all(registry.template storage<Entity>());
                auto validEntityView = std::views::filter([&registry](const Entity entity)
                                                          { return registry.valid(entity); });
                auto entityElementView = std::views::transform(
                    [&registry](const Entity entity)
                    {
                        if (registry.all_of<NameComponent>(entity))
                        {
                            auto& nameComponent = registry.get<NameComponent>(entity);
                            return std::format("{} ({})", nameComponent.Name.AsStringView(), entt::to_integral(entity));
                        }
                        return std::format("Entity {}", entt::to_integral(entity));
                    });

                for (const auto element : entityView | validEntityView | entityElementView)
                {
                    ImGui::Text("%s", element.c_str());
                }

                ImGui::TreePop();
            }
            ImGui::End();
        }
    }
} // namespace ig