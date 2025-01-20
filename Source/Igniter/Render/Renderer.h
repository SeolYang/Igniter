#pragma once
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"

namespace ig
{
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuTexture;
    class GpuView;
    class Window;
    class RenderContext;
    class MeshStorage;
    class SceneProxy;
    class CommandSignature;
    class TempConstantBufferAllocator;
    class World;

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

        [[nodiscard]] U8 GetMinMeshLod() const noexcept { return minMeshLod; }
        void SetMinMeshLod(U8 newMinMeshLod) { minMeshLod = newMinMeshLod; }

        [[nodiscard]] const TempConstantBufferAllocator* GetTempConstantBufferAllocator() const noexcept { return tempConstantBufferAllocator.get(); }

      private:
        const Window* window = nullptr;
        RenderContext* renderContext = nullptr;
        const MeshStorage* meshStorage = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        // #sy_test
        U8 minMeshLod = 0;

        Viewport mainViewport{};
        Ptr<ShaderBlob> vs;
        Ptr<ShaderBlob> ps;
        Ptr<RootSignature> bindlessRootSignature;
        Ptr<PipelineState> pso;
        eastl::array<RenderHandle<GpuTexture>, NumFramesInFlight> depthStencils;
        eastl::array<RenderHandle<GpuView>, NumFramesInFlight> dsvs;

        Ptr<GpuBuffer> uavCounterResetBuffer;

        Ptr<ShaderBlob> frustumCullingShader;
        Ptr<PipelineState> frustumCullingPso;
        InFlightFramesResource<RenderHandle<GpuBuffer>> cullingDataBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> cullingDataBufferSrv;
        InFlightFramesResource<RenderHandle<GpuView>> cullingDataBufferUav;

        Ptr<ShaderBlob> compactMeshLodInstancesShader;
        Ptr<PipelineState> compactMeshLodInstancesPso;

        Ptr<ShaderBlob> generateDrawInstanceCmdShader;
        Ptr<PipelineState> generateDrawInstanceCmdPso;
        constexpr static U32 kInitNumDrawCommands = 16;
        InFlightFramesResource<Ptr<GpuStorage>> meshLodInstanceStorage;
        InFlightFramesResource<GpuStorage::Allocation> meshLodInstanceSpace;
        InFlightFramesResource<Ptr<GpuStorage>> drawInstanceCmdStorage;
        InFlightFramesResource<GpuStorage::Allocation> drawInstanceCmdSpace;

        Ptr<CommandSignature> drawInstanceCmdSignature;

        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;
    };
} // namespace ig
