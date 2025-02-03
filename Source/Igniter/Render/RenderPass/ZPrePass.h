#pragma once
#include "Igniter/Render/Common.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    class CommandList;
    class GpuBuffer;
    class GpuTexture;
    class GpuView;
    class RootSignature;
    class DescriptorHeap;
    class ShaderBlob;
    class PipelineState;
    class RenderContext;
    class GpuStorage;
    class CommandSignature;

    struct ZPrePassParams
    {
        CommandList* CmdList = nullptr;
        CommandSignature* DrawInstanceCmdSignature = nullptr;
        RenderHandle<GpuBuffer> DrawInstanceCmdStorageBuffer;
        RenderHandle<GpuTexture> DepthTex;
        RenderHandle<GpuView> Dsv;
        Viewport MainViewport;
    };

    class ZPrePass : public RenderPass
    {
      public:
        ZPrePass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        ZPrePass(const ZPrePass&) = delete;
        ZPrePass(ZPrePass&&) noexcept = delete;

        ZPrePass& operator=(const ZPrePass&) = delete;
        ZPrePass& operator=(ZPrePass&&) noexcept = delete;

        void SetParams(const ZPrePassParams& newParams);

      protected:
       void OnExecute(const LocalFrameIndex localFrameIdx) override;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        ZPrePassParams params;

        Ptr<ShaderBlob> vs;
        Ptr<PipelineState> pso;
    };
} // namespace ig
