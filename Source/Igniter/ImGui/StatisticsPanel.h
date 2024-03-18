#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class StatisticsPanel : public ImGuiLayer
    {
    public:
        StatisticsPanel() = default;

        void Render() override;

    private:
        int pollingInterval = 60;
        int pollingStep = pollingInterval;

        double tempConstantBufferUsedSizeMB[2]{};
        double tempConstantBufferSizePerFrameMB{};
        float tempConstantBufferOccupancy[2]{};

        uint64_t handleManagerNumMemoryPools{};
        double handleManagerAllocatedChunkSizeMB{};
        double handleManagerUsedSizeMB{};
        uint64_t handleManagerNumAllocatedChunks{};
        uint64_t handleManagerNumAllocatedHandles{};
    };
} // namespace ig