#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    void PrintToDebugger(const std::string_view outputStr);

    class CallStack
    {
    private:
        constexpr static uint16_t kMaxNumBackTraceCapture = 16;

        struct CapturedFrames
        {
            void* Frames[kMaxNumBackTraceCapture]{};
            uint16_t NumCapturedFrames{0};
        };

    public:
        CallStack(const CallStack&) = delete;
        CallStack(CallStack&&) noexcept = delete;
        CallStack& operator=(const CallStack&) = delete;
        CallStack& operator=(CallStack&&) noexcept = delete;

        static U32 Capture();
        static std::string_view Dump(const DWORD captureHash);

    private:
        CallStack();
        ~CallStack();
        static CallStack& GetCallStack();

    private:
        HANDLE procHandle{nullptr};

        Mutex mutex{};
        UnorderedMap<DWORD, CapturedFrames> capturedFramesTable{};
        UnorderedMap<DWORD, std::string> dumpCache{};
    };
} // namespace ig
