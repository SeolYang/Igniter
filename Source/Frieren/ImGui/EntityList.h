#pragma once
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class EntityList : public ImGuiLayer
    {
    public:
        EntityList();

        void Render() override;

    };
}
