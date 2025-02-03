#pragma once
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

    struct TestForwardShadingPassParams
    {
        CommandList* CmdList = nullptr;
        CommandSignature* DrawInstanceCmdSignature = nullptr;
        RenderHandle<GpuBuffer> DrawInstanceCmdStorageBuffer;
        RenderHandle<GpuTexture> DepthTex;
        RenderHandle<GpuView> Dsv;
        Viewport MainViewport;
    };

    class ShaderBlob;
    class PipelineState;
    class TestForwardShadingPass : public RenderPass
    {
      public:
        TestForwardShadingPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, const Viewport& mainViewport);
        TestForwardShadingPass(const TestForwardShadingPass&) = delete;
        TestForwardShadingPass(TestForwardShadingPass&&) noexcept = delete;
        ~TestForwardShadingPass() override;

        TestForwardShadingPass& operator=(const TestForwardShadingPass&) = delete;
        TestForwardShadingPass& operator=(TestForwardShadingPass&&) noexcept = delete;

        void SetParams(const TestForwardShadingPassParams newParams);

        [[nodiscard]]
        RenderHandle<GpuTexture> GetOutputTex(const LocalFrameIndex localFrameIdx) const noexcept
        {
            return outputTex[localFrameIdx];
        }
        [[nodiscard]]
        RenderHandle<GpuView> GetOutputTexSrv(const LocalFrameIndex localFrameIdx) const noexcept
        {
            return outputTexSrv[localFrameIdx];
        }

      protected:
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

      private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        TestForwardShadingPassParams params;

        Ptr<ShaderBlob> vs;
        Ptr<ShaderBlob> ps;
        Ptr<PipelineState> pso;

        InFlightFramesResource<RenderHandle<GpuTexture>> outputTex;
        InFlightFramesResource<RenderHandle<GpuView>> outputTexRtv;
        InFlightFramesResource<RenderHandle<GpuView>> outputTexSrv;
    };
} // namespace ig
