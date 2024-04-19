#pragma once
#include <Frieren.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class StatisticsPanel final : public ImGuiLayer
    {
    public:
        StatisticsPanel()                           = default;
        StatisticsPanel(const StatisticsPanel&)     = delete;
        StatisticsPanel(StatisticsPanel&&) noexcept = delete;
        ~StatisticsPanel() override                 = default;

        StatisticsPanel& operator=(const StatisticsPanel&) = delete;
        StatisticsPanel& operator=(StatisticsPanel&&)      = delete;

        void Render() override;

    private:
        bool bEnablePolling  = true;
        int  pollingInterval = 60;
        int  pollingStep     = pollingInterval;

        double tempConstantBufferUsedSizeMB[2]{};
        double tempConstantBufferSizePerFrameMB{};
        float  tempConstantBufferOccupancy[2]{};

        uint64_t handleManagerNumMemoryPools{};
        double   handleManagerAllocatedChunkSizeMB{};
        double   handleManagerUsedSizeMB{};
        uint64_t handleManagerNumAllocatedChunks{};
        uint64_t handleManagerNumAllocatedHandles{};
    };
} // namespace ig
