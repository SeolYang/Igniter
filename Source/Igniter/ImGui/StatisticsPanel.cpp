#include <Core/Igniter.h>
#include <Core/Timer.h>
#include <ImGui/Common.h>
#include <ImGui/StatisticsPanel.h>
#include <Render/Renderer.h>
#include <Render/TempConstantBufferAllocator.h>

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
                ImGui::TreePop();
            }

            Renderer& renderer = Igniter::GetRenderer();
            if (ImGui::TreeNodeEx("Temp Constant Buffer Occupancy", ImGuiTreeNodeFlags_Framed))
            {
                TempConstantBufferAllocator& tempConstantBufferAllocator = renderer.GetTempConstantBufferAllocator();
                const auto [tempCBufferAllocLocalFrame0, tempCBufferAllocLocalFrame1] = tempConstantBufferAllocator.GetUsedSizeInBytes();
                const uint32_t tempCBufferReservedCapacityPerFrame = tempConstantBufferAllocator.GetReservedSizeInBytesPerFrame();
                ImGui::Text("Local Frame#0 Used: %d bytes / %d bytes", tempCBufferAllocLocalFrame0, tempCBufferReservedCapacityPerFrame);
                ImGui::ProgressBar(tempCBufferAllocLocalFrame0 / static_cast<float>(tempCBufferReservedCapacityPerFrame), ImVec2(0, 0));
                ImGui::Text("Local Frame#1 Used: %d bytes / %d bytes", tempCBufferAllocLocalFrame1, tempCBufferReservedCapacityPerFrame);
                ImGui::ProgressBar(tempCBufferAllocLocalFrame1 / static_cast<float>(tempCBufferReservedCapacityPerFrame), ImVec2(0, 0));
                ImGui::TreePop();
            }
            ImGui::End();
        }
    }
} // namespace ig