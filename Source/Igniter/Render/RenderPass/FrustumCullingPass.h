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

        [[nodiscard]] RenderHandle<GpuView> GetMeshLodInstanceStorageUav() const noexcept { return meshLodInstanceStorage->GetUnorderedResourceView(); }
        [[nodiscard]] RenderHandle<GpuBuffer> GetCullingDataBuffer() const noexcept { return cullingDataBuffer; }
        [[nodiscard]] RenderHandle<GpuView> GetCullingDataBufferSrv() const noexcept { return cullingDataBufferSrv; }

      protected:
        void PreExecute(const LocalFrameIndex localFrameIdx) override;
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

      public:
        constexpr static Size kNumInitMeshLodInstances = 256;
        constexpr static U32 kNumThreadsPerGroup = 32;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        FrustumCullingPassParams params;

        Ptr<ShaderBlob> shader;
        Ptr<PipelineState> pso;

        Ptr<GpuStorage> meshLodInstanceStorage;

        RenderHandle<GpuBuffer> cullingDataBuffer;
        RenderHandle<GpuView> cullingDataBufferSrv;
        RenderHandle<GpuView> cullingDataBufferUav;
    };
} // namespace ig
