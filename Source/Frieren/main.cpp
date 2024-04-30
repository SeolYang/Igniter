#include <Frieren.h>
#include <Application/TestApp.h>

int main()
{
    fe::TestApp app{ig::AppDesc{.WindowTitle = "Test", .WindowWidth = 1920, .WindowHeight = 1080}};
    return app.Execute();
}

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

struct MemoryLeakDetector
{
    MemoryLeakDetector()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    }

    ~MemoryLeakDetector() { _CrtDumpMemoryLeaks(); }
};

#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
MemoryLeakDetector memLeakDetector;
#pragma warning(pop)
#endif