#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"

namespace ig
{
    struct AppDesc
    {
        String WindowTitle;
        uint32_t WindowWidth, WindowHeight;
    };

    class Igniter;
    class Application
    {
    public:
        Application(const Application&) = delete;
        Application(Application&&) noexcept = delete;
        virtual ~Application();

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) noexcept = delete;

        int Execute();

        virtual void Update() = 0;
        virtual void Render() = 0;

    protected:
        Application(const AppDesc& desc);

    private:
        Ptr<Igniter> engine;

    };

}    // namespace ig