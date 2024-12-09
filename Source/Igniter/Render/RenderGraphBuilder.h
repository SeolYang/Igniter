/* !!!!!!!!!!!!!!!!!!! 간소화를 위해 더 이상 활용하지 말 것 !!!!!!!!!!!!!!!!!!! */
#pragma once
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/RederGraphResource.h"
#include "Igniter/Render/RenderPass.h"
#include "Igniter/Render/RenderGraph.h"

namespace ig
{
    class GpuTexture;
    class GpuBuffer;
}    // namespace ig

namespace ig
{
    // 일회용 인스턴스로 디자인 할 것
    // 실제로 빌드 된 다음, RenderGraph에 필요한 모든 자원 소유권을 넘길 것.
    // Render Pass는 순차적으로 추가된다고 가정
    // 항상 Write/Read 메서드들은 가장 마지막에 추가된 렌더 패스에 대한 연산이라고 가정
    /* #sy_todo Render Pass Culling */
    class RenderGraphBuilder final
    {
        friend class RenderGraph;

        // 만약에 Writer도 Reader도 없으면 Orphan Resource -> Culling?
        struct ResourceDependency
        {
        public:
            bool HasWriter() const noexcept { return Writer != 0xffffffffffffffffUi64; }

        public:
            UnorderedSet<size_t> Readers{};
            UnorderedSet<size_t> Owners{};
            size_t Writer{0xffffffffffffffffUi64};
            D3D12_BARRIER_LAYOUT Layout{D3D12_BARRIER_LAYOUT_UNDEFINED};
        };

    public:
        RenderGraphBuilder(RenderContext& renderContext) : renderContext(renderContext) {}
        RenderGraphBuilder(const RenderGraphBuilder&) = delete;
        RenderGraphBuilder(RenderGraphBuilder&&) noexcept = delete;
        ~RenderGraphBuilder() = default;

        RenderGraphBuilder operator=(const RenderGraphBuilder&) = delete;
        RenderGraphBuilder operator=(RenderGraphBuilder&&) noexcept = delete;

        template <typename Pass, typename... Args>
            requires std::derived_from<Pass, RenderPass>
        Pass& AddPass(Args&&... args)
        {
            renderPasses.emplace_back(details::RGRenderPassPackage{.RenderPassPtr = MakePtr<Pass>(std::forward<Args>(args)...)});
            renderPasses.back().RenderPassPtr->Setup(*this);
            return static_cast<Pass&>(*renderPasses.back().RenderPassPtr);
        }

        void ExecuteOnAsyncCompute() { renderPasses.back().ExecuteOn = details::ERGExecutableQueue::AsyncCompute; }
        void ExecuteOnAsyncCopy() { renderPasses.back().ExecuteOn = details::ERGExecutableQueue::AsyncCopy; }

        Ptr<RenderGraph> Compile();

        RGResourceHandle CreateTexture(const GpuTextureDesc& desc);
        RGResourceHandle CloneTexture(const RGResourceHandle targetTex);
        RGResourceHandle CreateBuffer(const GpuBufferDesc& desc);
        RGResourceHandle CloneBuffer(const RGResourceHandle targetBuf);
        /* 등록 한 시점에 이미 텍스처 자원이 사전에 'currentLayout'으로 전환 되어 있어야 함. */
        RGResourceHandle RegisterExternalTexture(const RGTexture texture, const D3D12_BARRIER_LAYOUT currentLayout);
        RGResourceHandle RegisterExternalBuffer(const RGBuffer buffer);

        /* 전달된 핸들의 Subresource 항은 무시됨 */
        RGResourceHandle WriteTexture(const RGResourceHandle targetTex, const D3D12_BARRIER_LAYOUT targetLayout,
            const eastl::vector<RGTextureSubresouce>& subresources = {RGTextureSubresouce{}});
        RGResourceHandle WriteBuffer(const RGResourceHandle targetBuf);
        /* 전달된 핸들의 Subresource 항은 무시됨 */
        RGResourceHandle ReadTexture(const RGResourceHandle targetTex, const D3D12_BARRIER_LAYOUT targetLayout,
            const eastl::vector<RGTextureSubresouce>& subresources = {RGTextureSubresouce{}});
        RGResourceHandle ReadBuffer(const RGResourceHandle targetBuf);

    private:
        void BuildAdjanceyList();
        void BuildTopologicalOrderedRenderPassIndices();
        void TopologicalSortDFS(const size_t renderPassIdx, eastl::vector<bool>& visited, eastl::vector<bool>& onStack);
        void CalculateDependencyLevels();
        void BuildDependencyLevels();
        void BuildSyncPoints();

    private:
        RenderContext& renderContext;

        eastl::vector<details::RGRenderPassPackage> renderPasses{};

        eastl::vector<details::RGResource> resources{};
        /* 하나의 원본 핸들이 아니라, 각 Subresource나 버전별로 관리 */
        UnorderedMap<RGResourceHandle, ResourceDependency> resourceDependencies{};

        eastl::vector<size_t> topologicalOrderedRenderPassIndices{};

        uint16_t maxDependencyLevels{0};
        eastl::vector<details::RGDependencyLevel> dependencyLevels{};
        eastl::vector<details::RGSyncPoint> syncPoints{};
    };
}    // namespace ig