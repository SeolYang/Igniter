#include <ImGui/StatisticsPanel.h>
#include <ImGui/Common.h>
#include <Core/Igniter.h>
#include <Core/Timer.h>

namespace ig
{
    void StatisticsPanel::Render()
    {
        if (ImGui::Begin("Statistics Panel", &bIsVisible))
        {
            if (ImGui::TreeNodeEx("Engine", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                const Timer& timer = Igniter::GetTimer();
                ImGui::Text("FPS: %d", timer.GetStableFps());
                ImGui::Text("Delta Time: %d ms", timer.GetDeltaTimeMillis());
            }
            ImGui::End();
        }
    }
}