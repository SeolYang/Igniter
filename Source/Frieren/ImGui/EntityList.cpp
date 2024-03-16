#include <ImGui/EntityList.h>
#include <Gameplay/World.h>
#include <Core/NameComponent.h>

namespace fe
{
    EntityList::EntityList(World& world)
        : world(world),
          ImGuiLayer("Entity List"_fs)
    {
    }

    void EntityList::Render()
    {
        if (ImGui::Begin("List of Entity", &bIsVisible))
        {
            const String worldName = world.GetName();
            if (ImGui::TreeNode(worldName.AsStringView().data()))
            {
                auto& registry = world.GetRegistry();

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
} // namespace fe