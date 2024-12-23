#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    // @dependency	None
    class Timer final
    {
    public:
        Timer()                 = default;
        Timer(const Timer&)     = delete;
        Timer(Timer&&) noexcept = delete;
        ~Timer()                = default;

        Timer& operator=(const Timer&)     = delete;
        Timer& operator=(Timer&&) noexcept = delete;

        void Begin()
        {
            const auto now = std::chrono::high_resolution_clock::now();
            begin          = now;

            constexpr std::chrono::nanoseconds FrameCounterMeasureTime = std::chrono::seconds(1);
            const auto                         frameCounterElapsed     = now - frameCounterBegin;
            if (frameCounterElapsed >= FrameCounterMeasureTime)
            {
                frameCounterBegin = now;
                framePerSeconds   = static_cast<uint16_t>(frameCounter);
                frameCounter      = 0;
            }
        }

        void End()
        {
            const auto now = std::chrono::high_resolution_clock::now();
            deltaTime      = now - begin;

            ++frameCounter;
        }

        [[nodiscard]] uint64_t GetDeltaTimeNanos() const { return deltaTime.count(); }

        [[nodiscard]] uint64_t GetDeltaTimeMillis() const { return std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime).count(); }

        [[nodiscard]] double GetDeltaTimeF64() const { return GetDeltaTimeNanos() * 1e-09; }

        [[nodiscard]] float GetDeltaTime() const { return static_cast<float>(GetDeltaTimeF64()); }

        [[nodiscard]] uint16_t GetStableFps() const { return framePerSeconds; }

        template <typename T = std::chrono::milliseconds>
        static inline size_t Now()
        {
            return std::chrono::duration_cast<T>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

    private:
        std::chrono::steady_clock::time_point begin     = std::chrono::high_resolution_clock::now();
        std::chrono::nanoseconds              deltaTime = std::chrono::nanoseconds(0);

        uint64_t                                       frameCounter      = 0;
        uint16_t                                       framePerSeconds   = 0;
        std::chrono::high_resolution_clock::time_point frameCounterBegin = std::chrono::high_resolution_clock::now();
    };

    struct TempTimer
    {
    public:
        void   Begin() { begin = Timer::Now(); }
        size_t End() const { return Timer::Now() - begin; }

    public:
        size_t begin = 0;
    };
} // namespace ig
