#pragma once
#include <ImGui/Common.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class World;
    class EntityList : public ImGuiLayer
    {
    public:
        EntityList(World& world);

        void Render() override;

    private:
        World& world;

    };
}
