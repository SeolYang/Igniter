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
    class ImGuiCanvas;
}    // namespace ig

namespace fe
{
    class RendererPrototype
    {
    public:
        RendererPrototype(ig::Window& window, ig::RenderContext& renderContext);
        RendererPrototype(const RendererPrototype&) = delete;
        RendererPrototype(RendererPrototype&&) noexcept = delete;
        ~RendererPrototype();

        RendererPrototype& operator=(const RendererPrototype&) = delete;
        RendererPrototype& operator=(RendererPrototype&&) noexcept = delete;

        /* Scene Data 설정 같은 방식으로 개선할 것! */
        void SetWorld(ig::World* newWorld) { this->world = newWorld; }
        void SetImGuiCanvas(ig::ImGuiCanvas* canvas) { this->imguiCanvas = canvas; }

        void PreRender(const ig::LocalFrameIndex localFrameIdx);
        ig::GpuSync Render(const ig::LocalFrameIndex localFrameIdx);

    private:
        ig::World* world = nullptr;
        ig::ImGuiCanvas* imguiCanvas = nullptr;
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