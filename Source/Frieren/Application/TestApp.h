#pragma once
#include "Igniter/Application/Application.h"

namespace ig
{
    class World;
    class GameSystem;
}    // namespace ig

namespace fe
{
    class TestGameSystem;
    class EditorCanvas;
    class TestApp : public ig::Application
    {
    public:
        TestApp(const AppDesc& desc);
        ~TestApp() override;

        void Update(const float deltaTime) override;
        void Render(const FrameIndex localFrameIdx) override;
        /* #sy_todo Implement Post Render */
        GpuSync PostRender([[maybe_unused]] const FrameIndex localFrameIdx) override { return GpuSync{};}

        void SetGameSystem(Ptr<ig::GameSystem> newGameSystem);

        ig::World* GetActiveWorld() { return world.get(); }

    private:
        /* #sy_todo Move Renderer to here!!! */
        Ptr<ig::World> world;
        Ptr<ig::GameSystem> gameSystem;
        Ptr<EditorCanvas> editorCanvas;
    };
}    // namespace fe