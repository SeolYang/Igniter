#pragma once
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    class Window;
    class RenderContext;
    class SceneProxy;
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuTexture;
    class GpuView;
    class TempConstantBufferAllocator;
    class Renderer final
    {
    public:
        Renderer(const Window& window, RenderContext& renderContext, const MeshStorage& meshStorage, const SceneProxy& sceneProxy);
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        ~Renderer();

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        void PreRender(const LocalFrameIndex localFrameIdx);
        GpuSyncPoint Render(const LocalFrameIndex localFrameIdx);
        GpuSyncPoint Render(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint sceneProxyRepSyncPoint);

        [[nodiscard]] const TempConstantBufferAllocator* GetTempConstantBufferAllocator() const noexcept { return tempConstantBufferAllocator.get(); }

    private:
        const Window* window = nullptr;
        RenderContext* renderContext = nullptr;
        const MeshStorage* meshStorage = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        Viewport mainViewport{};
        Ptr<ShaderBlob> vs;
        Ptr<ShaderBlob> ps;
        Ptr<RootSignature> bindlessRootSignature;
        Ptr<PipelineState> pso;
        eastl::array<RenderHandle<GpuTexture>, NumFramesInFlight> depthStencils;
        eastl::array<RenderHandle<GpuView>, NumFramesInFlight> dsvs;

        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;
    };
} // namespace ig
