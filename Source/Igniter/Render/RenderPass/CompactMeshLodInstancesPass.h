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

    struct CompactMeshLodInstancesPassParams
    {
        CommandList* CmdList = nullptr;
        const GpuView* PerFrameCbvPtr = nullptr;
        RenderHandle<GpuView> MeshLodInstanceStorageUavHandle{};
        RenderHandle<GpuView> CullingDataBufferSrvHandle{};
        U32 NumRenderables = 0;
    };

    class CompactMeshLodInstancesPass : public RenderPass
    {
      public:
        CompactMeshLodInstancesPass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        CompactMeshLodInstancesPass(const CompactMeshLodInstancesPass&) = delete;
        CompactMeshLodInstancesPass(CompactMeshLodInstancesPass&&) noexcept = delete;
        ~CompactMeshLodInstancesPass() override;

        CompactMeshLodInstancesPass& operator=(const CompactMeshLodInstancesPass&) = delete;
        CompactMeshLodInstancesPass& operator=(CompactMeshLodInstancesPass&&) noexcept = delete;

        void SetParams(const CompactMeshLodInstancesPassParams newParams);

      protected:
        void Render(const LocalFrameIndex localFrameIdx) override;

      public:
        constexpr static U32 kNumThreadsPerGroup = 32;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        CompactMeshLodInstancesPassParams params;

        Ptr<ShaderBlob> shader;
        Ptr<PipelineState> pso;
    };
} // namespace ig
