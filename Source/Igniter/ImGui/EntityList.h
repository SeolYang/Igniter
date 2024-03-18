#pragma once
#include <ImGui/ImGuiLayer.h>
#include <Gameplay/Common.h>

namespace ig
{
    class EntityList : public ImGuiLayer
    {
    public:
        EntityList() = default;
        ~EntityList() = default;

        void Render() override;

        Entity GetSelectedEntity() const { return selectedEntity; }

    private:
        Entity selectedEntity = entt::null;
    };
} // namespace ig
