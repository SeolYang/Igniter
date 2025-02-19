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

    struct DispatchMeshInstanceParams
    {
        U32 MeshInstanceIdx;
        U32 TargetLevelOfDetail;
        U32 PerFrameParamsCbv;
    };

    struct DispatchMeshInstance
    {
        DispatchMeshInstanceParams Params;
        U32 ThreadGroupCountX;
        U32 ThreadGroupCountY;
        U32 ThreadGroupCountZ;
    };

    struct MeshInstancePassParams
    {
        CommandList* ComputeCmdList = nullptr;
        GpuBuffer* ZeroFilledBuffer = nullptr;
        const GpuView* PerFrameParamsCbv = nullptr;
        const GpuView* MeshInstanceIndicesBufferSrv = nullptr;
        GpuBuffer* DispatchOpaqueMeshInstanceStorageBuffer = nullptr;
        const GpuView* DispatchOpaqueMeshInstanceStorageUav = nullptr;
        GpuBuffer* DispatchTransparentMeshInstanceStorageBuffer = nullptr;
        const GpuView* DispatchTransparentMeshInstanceStorageUav = nullptr;
        U32 NumMeshInstances = 0;
        /* @todo 만약 Opaque/Transparent Mesh 정보가 개별적으로 필요한 경우 사용 할 수 있도록 준비 */
        const GpuView* OpaqueMeshInstanceIndicesStorageUav = nullptr;
        const GpuView* TransparentMeshInstanceIndicesStorageUav = nullptr;
    };

    struct MeshInstancePassConstants
    {
        U32 PerFrameParamsCbv;
        U32 MeshInstanceIndicesBufferSrv;
        U32 NumMeshInstances;

        U32 OpaqueMeshInstanceDispatchBufferUav;
        U32 TransparentMeshInstanceDispatchBufferUav;
    };

    class MeshInstancePass : public RenderPass
    {
    public:
        MeshInstancePass(RenderContext& renderContext, RootSignature& bindlessRootSignature);
        MeshInstancePass(const MeshInstancePass&) = delete;
        MeshInstancePass(MeshInstancePass&&) noexcept = delete;
        ~MeshInstancePass() override;

        MeshInstancePass& operator=(const MeshInstancePass&) = delete;
        MeshInstancePass& operator=(MeshInstancePass&&) noexcept = delete;

        void SetParams(const MeshInstancePassParams& newParams);

    protected:
        void OnRecord(const LocalFrameIndex localFrameIdx) override;

    private:
        RenderContext* renderContext;
        RootSignature* bindlessRootSignature;

        MeshInstancePassParams params;

        Ptr<ShaderBlob> cs;
        Ptr<PipelineState> pso;
    };
}
