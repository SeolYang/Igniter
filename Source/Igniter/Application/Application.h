#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/GpuSync.h"

namespace ig
{
    struct AppDesc
    {
        String WindowTitle;
        uint32_t WindowWidth, WindowHeight;
    };

    class Engine;
    class Application
    {
    public:
        Application(const Application&) = delete;
        Application(Application&&) noexcept = delete;
        virtual ~Application();

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) noexcept = delete;

        int Execute();

        virtual void PreUpdate(const float deltaTime) = 0;
        virtual void Update(const float deltaTime) = 0;
        virtual void PostUpdate(const float deltaTime) = 0;

        virtual void PreRender(const LocalFrameIndex localFrameIdx) = 0;
        virtual GpuSync Render(const LocalFrameIndex localFrameIdx) = 0;
        virtual void PostRender(const LocalFrameIndex localFrameIdx) = 0;

    protected:
        Application(const AppDesc& desc);

    private:
        Ptr<Engine> engine;
    };
}    // namespace ig