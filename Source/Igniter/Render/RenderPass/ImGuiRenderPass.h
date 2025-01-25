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

    struct ImGuiRenderPassParams
    {
        CommandList* CmdList = nullptr;
        RenderHandle<GpuTexture> BackBuffer;
        RenderHandle<GpuView> BackBufferRtv;
        Viewport MainViewport;
    };

    class ImGuiRenderPass : public RenderPass
    {
      public:
        ImGuiRenderPass(RenderContext& renderContext);
        ImGuiRenderPass(const ImGuiRenderPass&) = delete;
        ImGuiRenderPass(ImGuiRenderPass&&) noexcept = delete;
        ~ImGuiRenderPass() override;

        ImGuiRenderPass& operator=(const ImGuiRenderPass&) = delete;
        ImGuiRenderPass& operator=(ImGuiRenderPass&&) noexcept = delete;

        void SetParams(const ImGuiRenderPassParams& newParams);

      protected:
        void Render(const LocalFrameIndex localFrameIdx) override;

      private:
        RenderContext* renderContext = nullptr;

        ImGuiRenderPassParams params;
    };
} // namespace ig
