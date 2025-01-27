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
        GpuSyncPoint Render(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint sceneProxyRepSyncPoint);

        [[nodiscard]] U8 GetMinMeshLod() const noexcept { return minMeshLod; }
        void SetMinMeshLod(U8 newMinMeshLod) { minMeshLod = newMinMeshLod; }

        /* #sy_todo 내용을 직접 밖으로 노출하지 말고, Statistics struct를 만들어서 채워서 내보내기 */
        [[nodiscard]] const TempConstantBufferAllocator* GetTempConstantBufferAllocator() const noexcept { return tempConstantBufferAllocator.get(); }

      private:
        const Window* window = nullptr;
        RenderContext* renderContext = nullptr;
        const MeshStorage* meshStorage = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        // #sy_test
        U8 minMeshLod = 0;

        Viewport mainViewport{};

        Ptr<RootSignature> bindlessRootSignature;
        eastl::array<RenderHandle<GpuTexture>, NumFramesInFlight> depthStencils;
        eastl::array<RenderHandle<GpuView>, NumFramesInFlight> dsvs;

        constexpr static Size kZeroFilledBufferSize = 512Ui64;
        Ptr<GpuBuffer> zeroFilledBuffer;

        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;

        Ptr<class LightClusteringPass> lightClusteringPass;
        Ptr<class FrustumCullingPass> frustumCullingPass;
        Ptr<class CompactMeshLodInstancesPass> compactMeshLodInstancesPass;
        Ptr<class GenerateMeshLodDrawCommandsPass> generateMeshLodDrawCmdsPass;
        Ptr<class TestForwardShadingPass> testForwardShadingPass;
        Ptr<class ImGuiRenderPass> imguiRenderPass;
    };
} // namespace ig
