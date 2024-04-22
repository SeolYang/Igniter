#include <Igniter.h>
#include <DbgHelp.h>
#include <debugapi.h>
#pragma comment(lib, "dbghelp.lib")
#include <Core/DebugTools.h>

namespace ig
{
    void PrintToDebugger(const std::string_view outputStr)
    {
        OutputDebugStringA(outputStr.data());
    }
}
