#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Render/RederGraphResource.h"
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    namespace details
    {
        struct RGResourceInstance
        {
        public:
            RGBuffer GetBuffer() const
            {
                if (Type != details::ERGResourceType::Buffer)
                {
                    IG_CHECK_NO_ENTRY();
                    return {};
                }

                return std::get<RGBuffer>(Resource);
            }

            RGTexture GetTexture() const
            {
                if (Type != details::ERGResourceType::Texture)
                {
                    IG_CHECK_NO_ENTRY();
                    return {};
                }

                return std::get<RGTexture>(Resource);
            }

        public:
            details::ERGResourceType Type;
            std::variant<RGBuffer, RGTexture> Resource;
            bool bIsExternal;
        };
    }    // namespace details

    class RenderContext;
    class RenderGraphBuilder;
    class RenderGraph final
    {
    public:
        RenderGraph(RenderGraphBuilder& builder);
        RenderGraph(const RenderGraph&) = delete;
        RenderGraph(RenderGraph&&) noexcept = delete;
        ~RenderGraph();

        RenderGraph& operator=(const RenderGraph&) = delete;
        RenderGraph& operator=(RenderGraph&&) noexcept = delete;

        GpuSync Execute(tf::Executor& taskExecutor);

        RGBuffer GetBuffer(const RGResourceHandle handle);
        RGTexture GetTexture(const RGResourceHandle handle);

        std::string Dump() const { return taskflow.dump(); }

    private:
        RenderContext& renderContext;

        eastl::vector<Ptr<RenderPass>> renderPasses{};
        eastl::vector<details::RGDependencyLevel> dependencyLevels{};
        eastl::vector<details::RGSyncPoint> syncPoints{};

        eastl::vector<details::RGResourceInstance> resourceInstances{};

        tf::Taskflow taskflow{"Render Graph"};

        eastl::vector<CommandContext*> mainGfxPendingCmdCtxList{};
        eastl::vector<CommandContext*> asyncComputePendingCmdCtxList{};
        eastl::vector<CommandContext*> asyncCopyPendingCmdCtxList{};

        GpuSync lastFrameSync;
    };
}    // namespace ig::experimental