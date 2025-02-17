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
        CommandList* GfxCmdList = nullptr;
        CommandSignature* DispatchMeshInstanceCmdSignature = nullptr;
        GpuBuffer* DispatchOpaqueMeshInstanceStorageBuffer = nullptr;
        const GpuView* Dsv = nullptr;
        Viewport MainViewport;
    };

    class ZPrePass : public RenderPass
    {
    public:
        ZPrePass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        ZPrePass(const ZPrePass&) = delete;
        ZPrePass(ZPrePass&&) noexcept = delete;
        ~ZPrePass();

        ZPrePass& operator=(const ZPrePass&) = delete;
        ZPrePass& operator=(ZPrePass&&) noexcept = delete;

        void SetParams(const ZPrePassParams& newParams);

    protected:
        void OnRecord(const LocalFrameIndex localFrameIdx) override;

    private:
        RenderContext* renderContext = nullptr;
        RootSignature* bindlessRootSignature = nullptr;

        ZPrePassParams params;

        Ptr<ShaderBlob> as;
        Ptr<ShaderBlob> ms;
        Ptr<PipelineState> pso;
    };
} // namespace ig
