#pragma once
#include "Frieren/Frieren.h"

namespace fe
{
    class StatisticsPanel final
    {
    public:
        StatisticsPanel()                           = default;
        StatisticsPanel(const StatisticsPanel&)     = delete;
        StatisticsPanel(StatisticsPanel&&) noexcept = delete;
        ~StatisticsPanel()                          = default;

        StatisticsPanel& operator=(const StatisticsPanel&) = delete;
        StatisticsPanel& operator=(StatisticsPanel&&)      = delete;

        void OnImGui();

    private:
        bool bEnablePolling  = true;
        int  pollingInterval = 60;
        int  pollingStep     = pollingInterval;

        double tempConstantBufferUsedSizeMB[2]{ };
        double tempConstantBufferSizePerFrameMB{ };
        float  tempConstantBufferOccupancy[2]{ };
    };
} // namespace fe
