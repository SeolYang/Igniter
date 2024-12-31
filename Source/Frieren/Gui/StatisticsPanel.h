#pragma once
#include "Frieren/Frieren.h"

namespace fe
{
    class Renderer;
    class StatisticsPanel final
    {
    public:
        explicit StatisticsPanel(const Renderer* renderer);
        StatisticsPanel(const StatisticsPanel&)     = delete;
        StatisticsPanel(StatisticsPanel&&) noexcept = delete;
        ~StatisticsPanel()                          = default;

        StatisticsPanel& operator=(const StatisticsPanel&) = delete;
        StatisticsPanel& operator=(StatisticsPanel&&)      = delete;

        void OnImGui();

    private:
        const Renderer* renderer        = nullptr;
        bool            bEnablePolling  = true;
        int             pollingInterval = 60;
        int             pollingStep     = pollingInterval;
    };
} // namespace fe
