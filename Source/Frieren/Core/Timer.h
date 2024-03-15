#pragma once
#include <chrono>

namespace fe
{
    // @dependency	None
    class Timer final
    {
    public:
        Timer() = default;
        Timer(const Timer&) = delete;
        Timer(Timer&&) noexcept = delete;
        ~Timer() = default;

        Timer& operator=(const Timer&) = delete;
        Timer& operator=(Timer&&) noexcept = delete;

        void Begin() { begin = std::chrono::high_resolution_clock::now(); }

        void End() { deltaTime = std::chrono::high_resolution_clock::now() - begin; }

        [[nodiscard]] uint64_t GetDeltaTimeNanos() const { return deltaTime.count(); }

        [[nodiscard]] uint64_t GetDeltaTimeMillis() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime).count();
        }

        [[nodiscard]] double GetDeltaTimeF64() const { return GetDeltaTimeNanos() * 1e-09; }

        [[nodiscard]] float GetDeltaTime() const { return static_cast<float>(GetDeltaTimeF64()); }

        template <typename T = std::chrono::milliseconds>
        static inline size_t Now()
        {
            return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }

    private:
        std::chrono::steady_clock::time_point begin = std::chrono::high_resolution_clock::now();
        std::chrono::nanoseconds deltaTime = std::chrono::nanoseconds(0);
    };

    struct TempTimer
    {
    public:
        void Begin() { begin = Timer::Now(); }
        size_t End() const { return Timer::Now() - begin; }

    public:
        size_t begin = 0;
    };
} // namespace fe