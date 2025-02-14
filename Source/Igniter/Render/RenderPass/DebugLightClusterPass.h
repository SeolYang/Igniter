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

    struct DebugLightClusterPassParams
    {
        CommandList* CmdList = nullptr;
        Viewport MainViewport;
        U32 NumLights = 0;

        const GpuView* PerFrameDataCbv = nullptr;
        Handle<GpuView> TileDwordsBufferSrv;
        Handle<GpuTexture> InputTex; // LAYOUT:RENDER_TARGET->SHADER_RESOURCE->COMMON
        Handle<GpuView> InputTexSrv;
    };

    class DebugLightClusterPass : public RenderPass
    {
    public:
        DebugLightClusterPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, const Viewport& mainViewport);
        DebugLightClusterPass(const DebugLightClusterPass&) = delete;
        DebugLightClusterPass(DebugLightClusterPass&&) noexcept = delete;
        ~DebugLightClusterPass() override;

        DebugLightClusterPass& operator=(const DebugLightClusterPass&) = delete;
        DebugLightClusterPass& operator=(DebugLightClusterPass&&) noexcept = delete;

        void SetParams(const DebugLightClusterPassParams& newParams);

        [[nodiscard]]
        Handle<GpuTexture> GetOutputTex() const noexcept
        {
            return outputTex;
        }

    protected:
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

    private:
        RenderContext* renderContext;
        RootSignature* bindlessRootSignature;

        DebugLightClusterPassParams params;

        Ptr<ShaderBlob> shader;
        Ptr<PipelineState> pso;

        Handle<GpuTexture> outputTex;
        Handle<GpuView> outputTexUav;
    };
} // namespace ig
