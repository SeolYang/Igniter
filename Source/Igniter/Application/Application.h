#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/GpuSyncPoint.h"

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

        virtual void PreUpdate([[maybe_unused]] const float deltaTime){};
        virtual void Update(const float deltaTime) = 0;
        virtual void PostUpdate([[maybe_unused]] const float deltaTime){};

        virtual void OnImGui(){};

        virtual void PreRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}
        virtual void PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}

    protected:
        Application(const AppDesc& desc);

    private:
        Ptr<Engine> engine;
    };
} // namespace ig
