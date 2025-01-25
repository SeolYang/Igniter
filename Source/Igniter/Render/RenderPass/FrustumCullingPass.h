#pragma once
#include "Igniter/Render/Common.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    class CommandList;
    class GpuBuffer;
    class GpuView;
    class RootSignature;
    class DescriptorHeap;
    class ShaderBlob;
    class PipelineState;
    class RenderContext;

    struct FrustumCullingPassParams
    {
        CommandList* CmdList = nullptr;
        GpuBuffer* ZeroFilledBufferPtr = nullptr;
        const GpuView* PerFrameCbvPtr = nullptr;
        Size NumRenderables = 0;
    };

    class FrustumCullingPass : public RenderPass
    {
      public:
        FrustumCullingPass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        FrustumCullingPass(const FrustumCullingPass&) = delete;
        FrustumCullingPass(FrustumCullingPass&&) noexcept = delete;
        ~FrustumCullingPass() override;

        FrustumCullingPass& operator=(const FrustumCullingPass&) = delete;
        FrustumCullingPass& operator=(FrustumCullingPass&&) noexcept = delete;

        void SetParams(const FrustumCullingPassParams& newParams);

        [[nodiscard]] RenderHandle<GpuView> GetMeshLodInstanceStorageUav(const LocalFrameIndex localFrameIdx) const noexcept { return meshLodInstanceStorage[localFrameIdx]->GetUnorderedResourceView(); }
        [[nodiscard]] RenderHandle<GpuBuffer> GetCullingDataBuffer(const LocalFrameIndex localFrameIdx) const noexcept { return cullingDataBuffer[localFrameIdx]; }
        [[nodiscard]] RenderHandle<GpuView> GetCullingDataBufferSrv(const LocalFrameIndex localFrameIdx) const noexcept { return cullingDataBufferSrv[localFrameIdx]; }

      protected:
        void PreRender(const LocalFrameIndex localFrameIdx) override;
        void Render(const LocalFrameIndex localFrameIdx) override;

      public:
        constexpr static Size kNumInitMeshLodInstances = 256;
        constexpr static U32 kNumThreadsPerGroup = 32;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        FrustumCullingPassParams params;

        Ptr<ShaderBlob> shader;
        Ptr<PipelineState> pso;

        InFlightFramesResource<Ptr<GpuStorage>> meshLodInstanceStorage;

        InFlightFramesResource<RenderHandle<GpuBuffer>> cullingDataBuffer;
        InFlightFramesResource<RenderHandle<GpuView>> cullingDataBufferSrv;
        InFlightFramesResource<RenderHandle<GpuView>> cullingDataBufferUav;
    };
} // namespace ig
