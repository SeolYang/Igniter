#pragma once
#include "Frieren/Frieren.h"

namespace ig
{
    class World;
}

namespace fe
{
    class EntityList;
    class EntityInspector final
    {
    private:
        struct ComponentInfo
        {
            entt::id_type ID;
            ig::meta::Type Type;
            ig::String NameToDisplay;
            ig::String RemoveButtonLabel;
            ig::String AttachSelectableLabel;
        };

    public:
        explicit EntityInspector(const EntityList& entityList);
        EntityInspector(const EntityInspector&) = delete;
        EntityInspector(EntityInspector&&) noexcept = delete;
        ~EntityInspector() = default;

        EntityInspector& operator=(const EntityInspector&) = delete;
        EntityInspector& operator=(EntityInspector&&) noexcept = delete;

        void OnImGui();
        void SetActiveWorld(ig::World* world) { activeWorld = world; }
        void EnableForceDirtyFlag() { bForceDirty = true; }

    private:
        void UpdateDirty(const ig::Entity selectedEntity);

    private:
        ig::World* activeWorld = nullptr;
        const EntityList* entityList;
        ig::Entity latestEntity = ig::NullEntity;

        eastl::vector<ComponentInfo> componentInfos;
        eastl::vector<size_t> componentInfoIndicesToDisplay;
        eastl::vector<size_t> addableComponentInfoIndices;
        bool bForceDirty = false;
        size_t componentToRemove = (size_t) -1;
    };
}    // namespace fe
