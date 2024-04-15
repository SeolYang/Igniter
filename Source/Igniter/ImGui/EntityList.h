#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class EntityList final : public ImGuiLayer
    {
    public:
        EntityList()  = default;
        ~EntityList() = default;

        void Render() override;

        Entity GetSelectedEntity() const { return selectedEntity; }

    private:
        Entity selectedEntity = entt::null;
    };
} // namespace ig
