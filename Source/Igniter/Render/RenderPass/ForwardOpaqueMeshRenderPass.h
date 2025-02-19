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

    struct ForwardOpaqueMeshRenderPassParams
    {
        CommandList* MainGfxCmdList = nullptr;
        GpuBuffer* DispatchOpaqueMeshInstanceStorageBuffer = nullptr;
        GpuTexture* RenderTarget = nullptr;
        const GpuView* RenderTargetView = nullptr;
        const GpuView* Dsv = nullptr;
        Viewport TargetViewport;
    };

    class ForwardOpaqueMeshRenderPass : public RenderPass
    {
    public:
        ForwardOpaqueMeshRenderPass(RenderContext& renderContext, RootSignature& bindlessRootSignature, CommandSignature& dispatchMeshInstanceCmdSignature);
        ForwardOpaqueMeshRenderPass(const ForwardOpaqueMeshRenderPass&) = delete;
        ForwardOpaqueMeshRenderPass(ForwardOpaqueMeshRenderPass&&) noexcept = delete;
        ~ForwardOpaqueMeshRenderPass() override;

        ForwardOpaqueMeshRenderPass& operator=(const ForwardOpaqueMeshRenderPass&) = delete;
        ForwardOpaqueMeshRenderPass& operator=(ForwardOpaqueMeshRenderPass&&) noexcept = delete;

        void SetParams(const ForwardOpaqueMeshRenderPassParams& newParams);

    protected:
        void OnRecord(const LocalFrameIndex localFrameIdx) override;

    private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;
        CommandSignature* dispatchMeshInstanceCmdSignature = nullptr;

        Ptr<ShaderBlob> as;
        Ptr<ShaderBlob> ms;
        Ptr<ShaderBlob> ps;
        Ptr<PipelineState> pso;

        ForwardOpaqueMeshRenderPassParams params;
    };
}
