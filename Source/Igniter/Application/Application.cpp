#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Application/Application.h"

namespace ig
{
    Application::Application(const AppDesc& desc)
    {
        const IgniterDesc engineDesc{ .WindowWidth = desc.WindowWidth, .WindowHeight = desc.WindowHeight, .WindowTitle = desc.WindowTitle };

        engine = MakePtr<Engine>(engineDesc);
    }

    Application::~Application() {}

    int Application::Execute()
    {
        return engine->Execute(*this);
    }

}    // namespace ig