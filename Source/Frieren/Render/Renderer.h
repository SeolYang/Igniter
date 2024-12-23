#pragma once
#include "Igniter/Render/Renderer.h"

namespace ig
{
    class Renderer;
    class World;
    class Window;
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuTexture;
    class GpuView;
    class TempConstantBufferAllocator;
    class ImGuiCanvas;
}    // namespace ig

namespace fe
{
    class Renderer : public ig::Renderer
    {
    public:
        Renderer(ig::Window& window, ig::RenderContext& renderContext);
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        virtual ~Renderer();

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        /* Scene Data 설정 같은 방식으로 개선할 것! */
        void SetWorld(ig::World* newWorld) { this->world = newWorld; }

        virtual void PreRender(const ig::LocalFrameIndex localFrameIdx) override;
        virtual ig::GpuSyncPoint Render(const ig::LocalFrameIndex localFrameIdx) override;

    private:
        ig::World* world = nullptr;
        ig::RenderContext& renderContext;

        ig::Viewport mainViewport{};
        ig::Ptr<ig::ShaderBlob> vs;
        ig::Ptr<ig::ShaderBlob> ps;
        ig::Ptr<ig::RootSignature> bindlessRootSignature;
        ig::Ptr<ig::PipelineState> pso;
        eastl::array<ig::RenderResource<ig::GpuTexture>, ig::NumFramesInFlight> depthStencils;
        eastl::array<ig::RenderResource<ig::GpuView>, ig::NumFramesInFlight> dsvs;

        ig::Ptr<ig::TempConstantBufferAllocator> tempConstantBufferAllocator;
    };
}    // namespace fe