#pragma once
#include "Igniter/Render/Common.h"
#include "Igniter/Render/Renderer.h"

namespace ig
{
    class World;
    class Window;
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuTexture;
    class GpuView;
    class TempConstantBufferAllocator;
}    // namespace ig

namespace fe
{
    class RendererPrototype : public Renderer
    {
    public:
        RendererPrototype(Window& window, RenderContext& renderContext);
        RendererPrototype(const RendererPrototype&) = delete;
        RendererPrototype(RendererPrototype&&) noexcept = delete;
        ~RendererPrototype();

        RendererPrototype& operator=(const RendererPrototype&) = delete;
        RendererPrototype& operator=(RendererPrototype&&) noexcept = delete;

        void PreRender(const LocalFrameIndex localFrameIdx) override;
        void Render(const LocalFrameIndex localFrameIdx, World& world) override;
        GpuSync PostRender(const LocalFrameIndex localFrameIdx) override;

    private:
        RenderContext& renderContext;

        Viewport mainViewport{};
        Ptr<ShaderBlob> vs;
        Ptr<ShaderBlob> ps;
        Ptr<RootSignature> bindlessRootSignature;
        Ptr<PipelineState> pso;
        eastl::array<RenderResource<GpuTexture>, NumFramesInFlight> depthStencils;
        eastl::array<RenderResource<GpuView>, NumFramesInFlight> dsvs;
        
        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;
    };
}    // namespace fe