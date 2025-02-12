#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Memory.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Renderer.h"
#include "Frieren/Gui/StatisticsPanel.h"

namespace fe
{

    StatisticsPanel::StatisticsPanel()
    {
    }

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

        ImGui::NewLine();

        if (ImGui::TreeNodeEx("GPU Stats", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto getColorFromPercentage = [](const double percentage)
            {
                if (percentage >= 0.9)
                {
                    return IM_COL32(255, 0, 0, 255);
                }

                if (percentage >= 0.5)
                {
                    return IM_COL32(255, 255, 0, 255);
                }

                return IM_COL32(0, 255, 0, 255);
            };

            const ig::GpuDevice::Statistics gpuStats = ig::Engine::GetRenderContext().GetGpuDevice().GetStatistics();
            ImGui::Text("GPU: %s", gpuStats.DeviceName.ToCString());
            ImGui::Text("GPU Type: %s", (gpuStats.bIsUma ? "Integrated GPU" : "Discrete GPU"));

            ImGui::Separator();

            const double dedicatedVideoMemOccupancy = gpuStats.DedicatedVideoMemUsage / (double)gpuStats.DedicatedVideoMemBudget;
            ImGui::PushStyleColor(ImGuiCol_Text, getColorFromPercentage(dedicatedVideoMemOccupancy));
            ImGui::Text("Dedicated Video Mem: %lf MB / %lf MB (%lf %%)",
                        ig::BytesToMegaBytes(gpuStats.DedicatedVideoMemUsage),
                        ig::BytesToMegaBytes(gpuStats.DedicatedVideoMemBudget),
                        dedicatedVideoMemOccupancy);
            ImGui::PopStyleColor(1);

            ImGui::Text("Dedicated Video Mem - Blocks: %llu(%lf MB)",
                        gpuStats.DedicatedVideoMemBlockCount,
                        ig::BytesToMegaBytes(gpuStats.DedicatedVideoMemBlockSize));
            ImGui::Text("Dedicated Video Mem - Allocations: %llu(%lf MB)",
                        gpuStats.DedicatedVideoMemAllocCount,
                        ig::BytesToMegaBytes(gpuStats.DedicatedVideoMemAllocSize));

            ImGui::NewLine();

            const double sharedVideoMemOccupacny = gpuStats.SharedVideoMemUsage / (double)gpuStats.SharedVideoMemBudget;
            ImGui::PushStyleColor(ImGuiCol_Text, getColorFromPercentage(dedicatedVideoMemOccupancy));
            ImGui::Text("Shared Video Mem: %lf MB / %lf MB (%lf %%)",
                        ig::BytesToMegaBytes(gpuStats.SharedVideoMemUsage),
                        ig::BytesToMegaBytes(gpuStats.SharedVideoMemBudget),
                        sharedVideoMemOccupacny);
            ImGui::PopStyleColor(1);

            ImGui::Text("Shared Video Mem - Blocks: %llu(%lf MB)",
                        gpuStats.SharedVideoMemBlockCount,
                        ig::BytesToMegaBytes(gpuStats.SharedVideoMemBlockSize));
            ImGui::Text("Shared Video Mem - Allocations: %llu(%lf MB)",
                        gpuStats.SharedVideoMemAllocCount,
                        ig::BytesToMegaBytes(gpuStats.SharedVideoMemAllocSize));

            ImGui::TreePop();
        }

        ImGui::NewLine();

        if (ImGui::TreeNodeEx("Temporary Constant Buffer Allocator", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            ig::Renderer& renderer = ig::Engine::GetRenderer();
            constexpr std::string_view FormatTemplate = "Local Frame#%d Used: %lf MB/%.01lf MB";

            const ig::TempConstantBufferAllocator* allocator = renderer.GetTempConstantBufferAllocator();
            IG_CHECK(allocator != nullptr);

            const double tempConstantBufferSizePerFrameInMB = ig::BytesToMegaBytes(allocator->GetReservedSizeInBytesPerFrame());
            for (ig::LocalFrameIndex localFrameIdx = 0; localFrameIdx < ig::NumFramesInFlight; ++localFrameIdx)
            {
                const double tempConstantBufferUsedSizeInMB = ig::BytesToMegaBytes(allocator->GetUsedSizeInBytes(localFrameIdx));
                const float tempConstantBufferOccupancy = static_cast<float>(tempConstantBufferUsedSizeInMB / tempConstantBufferSizePerFrameInMB);
                ImGui::Text(FormatTemplate.data(), localFrameIdx, tempConstantBufferUsedSizeInMB, tempConstantBufferSizePerFrameInMB);
                ImGui::ProgressBar(tempConstantBufferOccupancy, ImVec2(0, 0));
            }

            ImGui::TreePop();
        }
        ++pollingStep;
    }
} // namespace fe
