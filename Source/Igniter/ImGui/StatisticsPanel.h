#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class StatisticsPanel : public ImGuiLayer
    {
    public:
        StatisticsPanel() = default;

        void Render() override;

    };
}