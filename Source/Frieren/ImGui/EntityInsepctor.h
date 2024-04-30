#pragma once
#include <Frieren.h>

namespace ig
{
    class World;
}

namespace fe
{
    class EntityList;
    class EntityInspector final
    {
    public:
        explicit EntityInspector(const EntityList& entityList);
        EntityInspector(const EntityInspector&) = delete;
        EntityInspector(EntityInspector&&) noexcept = delete;
        ~EntityInspector() = default;

        EntityInspector& operator=(const EntityInspector&) = delete;
        EntityInspector& operator=(EntityInspector&&) noexcept = delete;

        void OnImGui();
        void SetActiveWorld(ig::World* world) { activeWorld = world; }

    private:
        ig::World* activeWorld = nullptr;
        const EntityList* entityList;
    };
}    // namespace fe
