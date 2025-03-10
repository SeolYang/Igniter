#pragma once
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Common.h"
#include "Igniter/Render/GpuStorage.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
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

    struct DepthPyramidConstants
    {
        U32 DepthPyramidSrv;
        U32 DepthPyramidSampler;
        U32 DepthPyramidWidth;
        U32 DepthPyramidHeight;
        U32 NumDepthPyramidMips;
    };

    struct RenderFrameSchedulingParams
    {
        LocalFrameIndex localFrameIdx;
        tf::Task BeginRender;
        tf::Task SceneProxyReplication;
    };

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

        /*
         * 내부에서 렌더링에 필요한 Task들을 모두 frame task flow에 스케줄링함.
         * @return FinalizeRenderTask
         */
        tf::Task ScheduleRenderTasks(tf::Taskflow& frameTaskflow, RenderFrameSchedulingParams renderFrameSchedulingParams);
        [[nodiscard]] GpuSyncPoint GetLocalFrarmeRenderSyncPoint() const noexcept { return localFrameRenderSyncPoint; }

        /* #sy_todo 내용을 직접 밖으로 노출하지 말고, Statistics struct를 만들어서 채워서 내보내기 */
        [[nodiscard]] const TempConstantBufferAllocator* GetTempConstantBufferAllocator() const noexcept { return tempConstantBufferAllocator.get(); }

    private:
        const Window* window = nullptr;
        RenderContext* renderContext = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        Viewport mainViewport{};

        Ptr<RootSignature> bindlessRootSignature; // #sy_todo render context로 이동시키는게 더 나아보임

        GpuCopyableFootprints depthBufferFootprints;
        Handle<GpuTexture> depthBuffer;
        Handle<GpuView> depthBufferDsv;

        /* Hi-Z Occlusion Culling for Forward Shading Pass */
        // @todo DepthPyramid 자체를 별도의 클래스로 빼내서 재사용 가능한 형태로 만들 것!
        DepthPyramidConstants depthPyramidConstants; 
        Handle<GpuTexture> depthPyramid;
        Vector<Uint2> depthPyramidExtents;
        Handle<GpuView> depthPyramidSrv;
        Vector<Handle<GpuView>> depthPyramidMipsUav;
        Handle<GpuView> depthPyramidSampler;
        GpuCopyableFootprints depthPyramidFootprints;
        /******************/
        
        Ptr<ShaderBlob> generateDepthPyramidShader;
        Ptr<PipelineState> generateDepthPyramidPso;

        constexpr static Size kZeroFilledBufferSize = 512Ui64;
        Ptr<GpuBuffer> zeroFilledBuffer;

        Ptr<TempConstantBufferAllocator> tempConstantBufferAllocator;

        Ptr<class LightClusteringPass> lightClusteringPass;
        Ptr<class PreMeshInstancePass> meshInstancePass;
        Ptr<class ZPrePass> zPrePass;
        Ptr<class ForwardOpaqueMeshRenderPass> forwardOpaqueMeshRenderPass;
        Ptr<class ImGuiRenderPass> imguiRenderPass;

        Ptr<CommandSignature> dispatchMeshInstanceCmdSignature;
        Ptr<ShaderBlob> meshInstancePassShader;
        Ptr<PipelineState> meshInstancePassPso;
        constexpr static U32 kInitDispatchBufferCapacity = 8192Ui64;
        Ptr<GpuStorage> opaqueMeshInstanceDispatchStorage;
        Ptr<GpuStorage> transparentMeshInstanceDispatchStorage;
        /* @pending 별도로 각 타입별로 분리된 Mesh Instance의 Indices를 저장 */
        //Handle<GpuBuffer> opaqueMeshInstanceBucket;
        //Handle<GpuBuffer> transparentMeshInstanceBucket;
        ////////////////////////////////////////////////////////////////

        TempConstantBuffer depthPyramidCb;
        TempConstantBuffer perFrameParamsCb;
        Matrix cpuCamViewMat{};
        Matrix gpuCamViewMat{};
        const GpuView* perFrameParamsCbvPtr = nullptr;
        
        GpuSyncPoint generateDepthPyramidSyncPoint;
        GpuSyncPoint lightClusteringSyncPoint;
        GpuSyncPoint preMeshInstancePassSyncPoint;
        GpuSyncPoint zPrePassSyncPoint;
        GpuSyncPoint forwardOpaqueMeshInstanceSyncPoint;
        GpuSyncPoint localFrameRenderSyncPoint;
        
    };
} // namespace ig
