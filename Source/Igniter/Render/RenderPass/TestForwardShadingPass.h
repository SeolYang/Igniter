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
        RenderHandle<GpuTexture> GetOutputTex() const noexcept
        {
            return outputTex;
        }
        [[nodiscard]]
        RenderHandle<GpuView> GetOutputTexSrv() const noexcept
        {
            return outputTexSrv;
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

        RenderHandle<GpuTexture> outputTex;
        RenderHandle<GpuView> outputTexRtv;
        RenderHandle<GpuView> outputTexSrv;
    };
} // namespace ig
