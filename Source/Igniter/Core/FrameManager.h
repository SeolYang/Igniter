#pragma once
#include <Igniter.h>

namespace ig
{
    class FrameManager
    {
    public:
        FrameManager() = default;
        FrameManager(const FrameManager&) = delete;
        FrameManager(FrameManager&&) noexcept = delete;
        ~FrameManager() = default;

        void NextFrame()
        {
            ++globalFrameIdx;
            localFrameIdx = globalFrameIdx % NumFramesInFlight;
        }

        size_t GetGlobalFrameIndex() const { return globalFrameIdx; }
        uint8_t GetLocalFrameIndex() const { return localFrameIdx; }

    private:
        size_t globalFrameIdx = 0;
        uint8_t localFrameIdx = 0;
    };
} // namespace ig
