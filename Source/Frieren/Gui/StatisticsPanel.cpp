#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Memory.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Frieren/Gui/StatisticsPanel.h"

namespace fe
{
    void StatisticsPanel::OnImGui()
    {
        if (bEnablePolling && pollingStep >= pollingInterval)
        {
            {
                /* #sy_todo Asset Manager나 RenderContext가 소유중인 HandleRegistry의 메모리 사용량을 추적 */
            }

            pollingStep = 0;
        }

        if (ImGui::RadioButton("##EnablePollingStatistics", bEnablePolling))
        {
            bEnablePolling = !bEnablePolling;
            pollingStep = 0;
        }

        ImGui::SameLine();

        ImGui::SliderInt("Interval", &pollingInterval, 10, 180);

        if (ImGui::TreeNodeEx("Engine (Real-time)", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            const ig::Timer& timer = ig::Engine::GetTimer();
            ImGui::Text("FPS: %d", timer.GetStableFps());
            ImGui::Text("Delta Time: %d ms", timer.GetDeltaTimeMillis());
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Temp Constant Buffer Occupancy", ImGuiTreeNodeFlags_Framed))
        {
            constexpr std::string_view FormatTemplate = "Local Frame#%d Used: %lf MB/%.01lf MB";
            ImGui::Text(FormatTemplate.data(), 0, tempConstantBufferUsedSizeMB[0], tempConstantBufferSizePerFrameMB);
            ImGui::ProgressBar(tempConstantBufferOccupancy[0], ImVec2(0, 0));
            ImGui::Text(FormatTemplate.data(), 1, tempConstantBufferUsedSizeMB[1], tempConstantBufferSizePerFrameMB);
            ImGui::ProgressBar(tempConstantBufferOccupancy[1], ImVec2(0, 0));
            ImGui::TreePop();
        }
        ++pollingStep;
    }
}    // namespace fe
