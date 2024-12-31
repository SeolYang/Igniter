#include "Frieren/Frieren.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Memory.h"
#include "Igniter/Core/Timer.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/MeshStorage.h"
#include "Frieren/Render/Renderer.h"
#include "Frieren/Gui/StatisticsPanel.h"

namespace fe
{

    StatisticsPanel::StatisticsPanel(const Renderer* renderer) :
        renderer(renderer)
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
            pollingStep    = 0;
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

        if (ImGui::TreeNodeEx("Temporary Constant Buffer Allocator", ImGuiTreeNodeFlags_Framed))
        {
            if (renderer != nullptr)
            {
                constexpr std::string_view FormatTemplate = "Local Frame#%d Used: %lf MB/%.01lf MB";

                const ig::TempConstantBufferAllocator* allocator = renderer->GetTempConstantBufferAllocator();
                IG_CHECK(allocator != nullptr);

                const double tempConstantBufferSizePerFrameInMB = ig::BytesToMegaBytes(allocator->GetReservedSizeInBytesPerFrame());
                for (ig::LocalFrameIndex localFrameIdx = 0; localFrameIdx < ig::NumFramesInFlight; ++localFrameIdx)
                {
                    const double tempConstantBufferUsedSizeInMB = ig::BytesToMegaBytes(allocator->GetUsedSizeInBytes(localFrameIdx));
                    const float  tempConstantBufferOccupancy    = static_cast<float>(tempConstantBufferUsedSizeInMB / tempConstantBufferSizePerFrameInMB);
                    ImGui::Text(FormatTemplate.data(), localFrameIdx, tempConstantBufferUsedSizeInMB, tempConstantBufferSizePerFrameInMB);
                    ImGui::ProgressBar(tempConstantBufferOccupancy, ImVec2(0, 0));
                }
            }
            else
            {
                ImGui::Text("Invalid");
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Mesh Storage", ImGuiTreeNodeFlags_Framed))
        {
            const ig::MeshStorage&            meshStorage           = ig::Engine::GetMeshStorage();
            const ig::MeshStorage::Statistics meshStorageStatistics = meshStorage.GetStatistics();

            const double staticMeshVertexStorageUsageInMB = ig::BytesToMegaBytes(meshStorageStatistics.StaticMeshVertexStorageUsage);
            const double staticMeshVertexStorageSizeInMB  = ig::BytesToMegaBytes(meshStorageStatistics.StaticMeshVertexStorageSize);
            const float  staticMeshVertexStorageOccupacny = static_cast<float>(staticMeshVertexStorageUsageInMB / staticMeshVertexStorageSizeInMB);
            ImGui::Text("Static Mesh Vertex Storage: %lf MB/%.01lf MB", staticMeshVertexStorageUsageInMB, staticMeshVertexStorageSizeInMB);
            ImGui::Text("Num Vertices: %lld", meshStorageStatistics.NumStaticMeshVertices);
            ImGui::ProgressBar(staticMeshVertexStorageOccupacny, ImVec2(0, 0));

            const double vertexIndexStorageUsageInMB = ig::BytesToMegaBytes(meshStorageStatistics.VertexIndexStorageUsage);
            const double vertexIndexStorageSizeInMB  = ig::BytesToMegaBytes(meshStorageStatistics.VertexIndexStorageSize);
            const float  vertexIndexStorageOccupacny = static_cast<float>(vertexIndexStorageUsageInMB / vertexIndexStorageSizeInMB);
            ImGui::Text("Vertex Index Storage: %lf MB/%.01lf MB", vertexIndexStorageUsageInMB, vertexIndexStorageSizeInMB);
            ImGui::Text("Num Indices: %lld", meshStorageStatistics.NumVertexIndices);
            ImGui::ProgressBar(vertexIndexStorageOccupacny, ImVec2(0, 0));

            ImGui::TreePop();
        }
        ++pollingStep;
    }
} // namespace fe
