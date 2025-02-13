#pragma once
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/Mesh.h"

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
        Renderer(const Window& window, RenderContext& renderContext, const SceneProxy& sceneProxy);
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        ~Renderer();

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        void PreRender(const LocalFrameIndex localFrameIdx);
        GpuSyncPoint Render(const LocalFrameIndex localFrameIdx, const World& world, GpuSyncPoint sceneProxyRepSyncPoint);

        /* @test */
        [[nodiscard]] U8 GetMinMeshLod() const noexcept { return minMeshLod; }
        void SetMinMeshLod(const U8 newMinMeshLod) { minMeshLod = std::min(newMinMeshLod, Mesh::kMaxMeshLevelOfDetails); }

        /* #sy_todo 내용을 직접 밖으로 노출하지 말고, Statistics struct를 만들어서 채워서 내보내기 */
        [[nodiscard]] const TempConstantBufferAllocator* GetTempConstantBufferAllocator() const noexcept { return tempConstantBufferAllocator.get(); }

    private:
        const Window* window = nullptr;
        RenderContext* renderContext = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        /* @test */
        U8 minMeshLod = 0;

        Viewport mainViewport{};

        Ptr<RootSignature> bindlessRootSignature; // #sy_todo render context로 이동시키는게 더 나아보임
        Handle<GpuTexture> depthStencil;
        Handle<GpuView> dsv;

        constexpr static Size kZeroFilledBufferSize = 512Ui64;
        Ptr<GpuBuffer> zeroFilledBuffer;

        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;
        Ptr<class ImGuiRenderPass> imguiRenderPass;

        /* @test Mesh Instance Indices를 통한 Culling/LOD 선택/IndirectCommandArguments 생성 테스트 */
        Ptr<ShaderBlob> meshInstancePassShader;
        Ptr<PipelineState> meshInstancePassPso;
        constexpr static U32 kInitDispatchBufferCapacity = 8192Ui64;
        Ptr<GpuStorage> opaqueMeshInstanceDispatchStorage;
        Ptr<GpuStorage> transparentMeshInstanceDispatchStorage;
        /* @pending 별도로 각 타입별로 분리된 Mesh Instance의 Indices를 저장 */
        //Handle<GpuBuffer> opaqueMeshInstanceBucket;
        //Handle<GpuBuffer> transparentMeshInstanceBucket;
        ////////////////////////////////////////////////////////////////
        /* HLSL.DispatchMeshInstance 및 HLSL.DispatchMeshInstancePayload 구조체 참고 */
        Ptr<CommandSignature> meshInstanceDispatchCmdSignature;
        /************************************/
    };
} // namespace ig
