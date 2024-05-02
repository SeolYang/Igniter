#pragma once
#include "Igniter/Application/Application.h"

namespace ig
{
    class World;
    class GameSystem;
}    // namespace ig

namespace fe
{
    class RendererPrototype;
    class TestGameSystem;
    class EditorCanvas;
    class TestApp : public ig::Application
    {
    public:
        TestApp(const AppDesc& desc);
        ~TestApp() override;

        void PreUpdate(const float) override {}
        void Update(const float deltaTime) override;
        void PostUpdate(const float) override {}

        void PreRender(const LocalFrameIndex localFrameIdx) override;
        void Render(const LocalFrameIndex localFrameIdx) override;
        GpuSync PostRender(const LocalFrameIndex localFrameIdx) override;

        void SetGameSystem(Ptr<ig::GameSystem> newGameSystem);

        ig::World* GetActiveWorld() { return world.get(); }

    private:
        Ptr<RendererPrototype> renderer;
        Ptr<ig::World> world;
        Ptr<ig::GameSystem> gameSystem;
        Ptr<EditorCanvas> editorCanvas;
    };
}    // namespace fe