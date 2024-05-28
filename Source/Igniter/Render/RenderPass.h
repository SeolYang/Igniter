#pragma once
#include "Igniter/Core/String.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/Render/RederGraphResource.h"

namespace ig
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
        virtual void Execute(tf::Subflow& rnederPassSubflow, CommandContext& cmdCtx) = 0;

    private:
        String name;
    };
}    // namespace ig
