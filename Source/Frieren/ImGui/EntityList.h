#pragma once
#include "Frieren/Frieren.h"

namespace ig
{
    class World;
}

namespace fe
{
    class EntityList final
    {
    public:
        EntityList() = default;
        EntityList(const EntityList&) = delete;
        EntityList(EntityList&&) noexcept = delete;
        ~EntityList() = default;

        EntityList& operator=(const EntityList&) = delete;
        EntityList& operator=(EntityList&&) noexcept = delete;

        void OnImGui();
        Entity GetSelectedEntity() const { return selectedEntity; }

        void SetActiveWorld(ig::World* world)
        {
            selectedEntity = entt::null;
            activeWorld = world;
        }

    private:
        ig::World* activeWorld = nullptr;
        Entity selectedEntity = entt::null;
    };
}    // namespace fe
