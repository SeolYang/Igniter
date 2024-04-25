#include <Frieren.h>
#include <Core/Engine.h>
#include <Core/Memory.h>
#include <Core/Timer.h>
#include <Render/Renderer.h>
#include <Render/TempConstantBufferAllocator.h>
#include <ImGui/StatisticsPanel.h>

namespace fe
{
    void StatisticsPanel::Render()
    {
        if (bEnablePolling && pollingStep >= pollingInterval)
        {
            {
                Renderer& renderer = Igniter::GetRenderer();
                const TempConstantBufferAllocator& tempConstantBufferAllocator = renderer.GetTempConstantBufferAllocator();
                const auto [tempCBufferAllocLocalFrame0, tempCBufferAllocLocalFrame1] = tempConstantBufferAllocator.GetUsedSizeInBytes();
                tempConstantBufferUsedSizeMB[0] = BytesToMegaBytes(tempCBufferAllocLocalFrame0);
                tempConstantBufferUsedSizeMB[1] = BytesToMegaBytes(tempCBufferAllocLocalFrame1);

                tempConstantBufferSizePerFrameMB = BytesToMegaBytes(tempConstantBufferAllocator.GetReservedSizeInBytesPerFrame());
                tempConstantBufferOccupancy[0] = static_cast<float>(tempConstantBufferUsedSizeMB[0] / tempConstantBufferSizePerFrameMB);
                tempConstantBufferOccupancy[1] = static_cast<float>(tempConstantBufferUsedSizeMB[1] / tempConstantBufferSizePerFrameMB);
            }

            {
                /* #sy_todo Asset Manager나 RenderContext가 소유중인 HandleRegistry의 메모리 사용량을 추적 */
            }

            pollingStep = 0;
        }

        if (ImGui::Begin("Statistics Panel", &bIsVisible))
        {
            if (ImGui::RadioButton("##EnablePollingStatistics", bEnablePolling))
            {
                bEnablePolling = !bEnablePolling;
                pollingStep = 0;
            }

            ImGui::SameLine();

            ImGui::SliderInt("Interval", &pollingInterval, 10, 180);

            if (ImGui::TreeNodeEx("Engine (Real-time)", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                const Timer& timer = Igniter::GetTimer();
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
            ImGui::End();
        }

        ++pollingStep;
    }
}    // namespace fe
