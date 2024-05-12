#pragma once
#include "Igniter/Application/Application.h"

namespace ig
{
    class World;
    class GameSystem;
    class TempConstantBufferAllocator;
    class RenderGraph;
}    // namespace ig

namespace fe
{
    class MainRenderPass;
    class ImGuiPass;
    class BackBufferPass;
    class RendererPrototype;
    class TestGameSystem;
    class EditorCanvas;
    class TestApp : public ig::Application
    {
    public:
        TestApp(const ig::AppDesc& desc);
        ~TestApp() override;

        void PreUpdate(const float) override {}
        void Update(const float deltaTime) override;
        void PostUpdate(const float) override {}

        void PreRender(const ig::LocalFrameIndex localFrameIdx) override;
        ig::GpuSync Render(const ig::LocalFrameIndex localFrameIdx) override;
        void PostRender(const ig::LocalFrameIndex localFrameIdx) override;

        void SetGameSystem(ig::Ptr<ig::GameSystem> newGameSystem);

        ig::World* GetActiveWorld() { return world.get(); }

    private:
        tf::Executor& taskExecutor;

        ig::Ptr<ig::World> world;
        ig::Ptr<ig::TempConstantBufferAllocator> tempConstantBufferAllocator;
        ig::Ptr<ig::RenderGraph> renderGraph;
        ig::Ptr<ig::GameSystem> gameSystem;
        ig::Ptr<EditorCanvas> editorCanvas;

        MainRenderPass* mainRenderPass = nullptr;
        ImGuiPass* imGuiPass = nullptr;
        BackBufferPass* backBufferPass = nullptr;
    };
}    // namespace fe