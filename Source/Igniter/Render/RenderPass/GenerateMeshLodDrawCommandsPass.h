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
    class GpuStorage;
    class CommandSignature;

    struct GenerateMeshLodDrawCommandsPassParams
    {
        CommandList* CmdList = nullptr;
        const GpuView* PerFrameCbvPtr = nullptr;
        RenderHandle<GpuView> CullingDataBufferSrv;
        GpuBuffer* ZeroFilledBufferPtr = nullptr;
        U32 NumInstancing = 0;
    };

    class GenerateMeshLodDrawCommandsPass : public RenderPass
    {
      public:
        GenerateMeshLodDrawCommandsPass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        GenerateMeshLodDrawCommandsPass(const GenerateMeshLodDrawCommandsPass&) = delete;
        GenerateMeshLodDrawCommandsPass(GenerateMeshLodDrawCommandsPass&&) noexcept = delete;
        ~GenerateMeshLodDrawCommandsPass() override;

        GenerateMeshLodDrawCommandsPass& operator=(const GenerateMeshLodDrawCommandsPass&) = delete;
        GenerateMeshLodDrawCommandsPass& operator=(GenerateMeshLodDrawCommandsPass&&) = delete;

        void SetParams(const GenerateMeshLodDrawCommandsPassParams& newParams);

        [[nodiscard]] RenderHandle<GpuBuffer> GetDrawInstanceCmdStorageBuffer(const LocalFrameIndex localFrameIdx) const noexcept { return drawInstanceCmdStorage[localFrameIdx]->GetGpuBuffer(); }
        [[nodiscard]] CommandSignature* GetCommandSignature() noexcept { return cmdSignature.get(); }

      protected:
        void PreExecute(const LocalFrameIndex localFrameIdx) override;
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

      public:
        constexpr static U32 kNumInitDrawCommands = 128;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        GenerateMeshLodDrawCommandsPassParams params;

        Ptr<ShaderBlob> shader;
        Ptr<PipelineState> pso;

        InFlightFramesResource<Ptr<GpuStorage>> drawInstanceCmdStorage;

        Ptr<CommandSignature> cmdSignature;
    };
} // namespace ig
