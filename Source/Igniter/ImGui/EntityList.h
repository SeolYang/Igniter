#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class EntityList : public ImGuiLayer
    {
    public:
        EntityList();

        void Render() override;

    };
}
