#include <Igniter.h>
#include <Core/Engine.h>
#include <Application/Application.h>

namespace ig
{
    Application::Application(const AppDesc& desc)
    {
        const IgniterDesc engineDesc{
            .WindowWidth = desc.WindowWidth,
            .WindowHeight = desc.WindowHeight,
            .WindowTitle = desc.WindowTitle
        };

        engine = MakePtr<Igniter>(engineDesc);
    }

    Application::~Application() {}

    int Application::Execute() 
    {
        return engine->Execute(*this);
    }

}    // namespace ig