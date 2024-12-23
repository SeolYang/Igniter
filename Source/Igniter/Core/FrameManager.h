#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    class FrameManager final
    {
    public:
        FrameManager(const FrameManager&)     = delete;
        FrameManager(FrameManager&&) noexcept = delete;
        ~FrameManager()                       = default;

        static GlobalFrameIndex GetGlobalFrameIndex() { return GetInstance().globalFrameIdx; }
        static LocalFrameIndex  GetLocalFrameIndex() { return GetInstance().localFrameIdx; }

        static LocalFrameIndex BeginFrame() { return GetInstance().localFrameIdx; }

        static void EndFrame()
        {
            FrameManager& frameManager = GetInstance();
            ++frameManager.globalFrameIdx;
            frameManager.localFrameIdx = frameManager.globalFrameIdx % NumFramesInFlight;
        }

    private:
        FrameManager() = default;

        static FrameManager& GetInstance()
        {
            static FrameManager frameManager{ };
            return frameManager;
        }

    private:
        GlobalFrameIndex globalFrameIdx = 0;
        LocalFrameIndex  localFrameIdx  = 0;
    };
} // namespace ig
