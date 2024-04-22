#include <Igniter.h>
#include <DbgHelp.h>
#include <debugapi.h>
#pragma comment(lib, "dbghelp.lib")
#include <Core/DebugTools.h>

namespace ig::details
{
    template <uint32_t BufferSize = 256>
    struct SymbolInfo : public SYMBOL_INFO
    {
    public:
        SymbolInfo()
        {
            MaxNameLen   = BufferSize;
            SizeOfStruct = sizeof(SYMBOL_INFO);
        }

    private:
        char buffer[BufferSize]{};
    };
}

namespace ig
{
    void PrintToDebugger(const std::string_view outputStr)
    {
        OutputDebugStringA(outputStr.data());
    }

    CallStack::CallStack()
    {
        const HANDLE currentProc{GetCurrentProcess()};
        if (DuplicateHandle(currentProc, currentProc, currentProc, &procHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            SymSetOptions(SYMOPT_LOAD_LINES);
            if (!SymInitialize(procHandle, nullptr, TRUE))
            {
                PrintToDebugger("Failed to initialize symbols.");
            }
        }
        else
        {
            PrintToDebugger("Failed to duplicate process handle.");
        }
    }

    CallStack::~CallStack()
    {
        if (procHandle != GetCurrentProcess() && procHandle != nullptr)
        {
            SymCleanup(procHandle);
        }
    }

    CallStack& CallStack::GetCallStack()
    {
        static CallStack callStack{};
        return callStack;
    }

    uint32_t CallStack::Capture()
    {
        CallStack&     callStack = GetCallStack();
        CapturedFrames frames{};
        DWORD          backTraceHash{};
        frames.NumCapturedFrames = CaptureStackBackTrace(1, MaxNumBackTraceCapture, &frames.Frames[0], &backTraceHash);

        {
            UniqueLock lock{callStack.mutex};
            callStack.capturedFramesTable[backTraceHash] = frames;
        }
        return backTraceHash;
    }

    std::string_view CallStack::Dump(const DWORD captureHash)
    {
        CallStack&       callStack = GetCallStack();
        std::string_view dumped{};

        UniqueLock lock{callStack.mutex};
        if (!callStack.dumpCache.contains(captureHash))
        {
            if (!callStack.capturedFramesTable.contains(captureHash))
            {
                return {};
            }

            if (callStack.procHandle == nullptr)
            {
                PrintToDebugger("Symbol lookup table does not initialized.");
                return {};
            }

            const CapturedFrames& capturedFrames = callStack.capturedFramesTable[captureHash];
            std::ostringstream    outputStream{};
            for (size_t idx = 0; idx < capturedFrames.NumCapturedFrames; ++idx)
            {
                const void* const frame = capturedFrames.Frames[idx];
                if (frame == nullptr)
                {
                    continue;
                }

                const DWORD64       frameAddr = reinterpret_cast<DWORD64>(frame);
                details::SymbolInfo symbolInfo{};
                SymFromAddr(callStack.procHandle, frameAddr, nullptr, &symbolInfo);

                DWORD           displacement{0};
                IMAGEHLP_LINE64 line{.SizeOfStruct = sizeof(IMAGEHLP_LINE64)};
                if (SymGetLineFromAddr64(callStack.procHandle, frameAddr, &displacement, &line))
                {
                    outputStream << std::format("{} <= Line: {}; File: {}\n", symbolInfo.Name, line.LineNumber,
                                                line.FileName);
                }
                else
                {
                    outputStream << std::format("{} <= <Unknown>\n", symbolInfo.Name);
                }
            }

            callStack.dumpCache[captureHash] = std::move(outputStream).str();
        }

        dumped = callStack.dumpCache[captureHash];
        return dumped;
    }
}
