#pragma once
#include "Igniter/Render/Common.h"
#include "Igniter/Render/RenderPass.h"

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
    class RendererPrototype : public ig::RenderPass
    {
    public:
        RendererPrototype(ig::Window& window, ig::RenderContext& renderContext);
        RendererPrototype(const RendererPrototype&) = delete;
        RendererPrototype(RendererPrototype&&) noexcept = delete;
        ~RendererPrototype();

        RendererPrototype& operator=(const RendererPrototype&) = delete;
        RendererPrototype& operator=(RendererPrototype&&) noexcept = delete;

        void PreRender(const ig::LocalFrameIndex localFrameIdx) override;
        void Render(const ig::LocalFrameIndex localFrameIdx, ig::World& world) override;
        ig::GpuSync PostRender(const ig::LocalFrameIndex localFrameIdx) override;

    private:
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