#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/Render/RenderGraph/RederGraphResource.h"

namespace ig::experimental
{
    class RenderGraph;
    class RenderGraphBuilder;
    class RenderPass
    {
    public:
        RenderPass(const String name) noexcept : name(name) {}
        virtual ~RenderPass() = default;

        String GetName() const noexcept { return name; }

        virtual void Setup(RenderGraphBuilder& builder) = 0;

        /* 'PostCompile'에서 실제로 생성된 자원을 얻고, 이들에 대한 Gpu View를 생성 할 것! */
        virtual void PostCompile(RenderGraph& renderGraph) = 0;

        /* 'Render Pass Subflow'에서 다시 한번 작업을 분할 할 수 있다. 일반적으로, 1개 파생 서브 플로우에서 1개의 CommandContext를 맡도록! */
        // #sy_note 활용 방식
        // https://taskflow.github.io/taskflow/classtf_1_1FlowBuilder.html#aae3edfa278baa75b08414e083c14c836
        // https://taskflow.github.io/taskflow/classtf_1_1GuidedPartitioner.html
        // for_each_task = subflow.for_each_index(...); pendingCmdCtxList.resize(for_each_task.num_successors()) ->
        // Num elements per worker ==> (end-begin)/pendingCmdCtxList.size(), pendingListIdx = idx/NumElementsPerWorker
        virtual void Execute(tf::Subflow& renderPassSubflow, eastl::vector<CommandContext*>& pendingCmdCtxList) = 0;

    private:
        String name;
    };
}    // namespace ig::experimental
