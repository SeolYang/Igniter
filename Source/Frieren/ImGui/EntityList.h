#pragma once
#include <Frieren.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class EntityList final : public ImGuiLayer
    {
    public:
        EntityList() = default;
        EntityList(const EntityList&) = delete;
        EntityList(EntityList&&) noexcept = delete;
        ~EntityList() override = default;

        EntityList& operator=(const EntityList&) = delete;
        EntityList& operator=(EntityList&&) noexcept = delete;

        void Render() override;

        Entity GetSelectedEntity() const { return selectedEntity; }

    private:
        Entity selectedEntity = entt::null;
    };
}    // namespace fe
