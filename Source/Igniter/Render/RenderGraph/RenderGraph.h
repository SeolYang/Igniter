#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Render/RenderGraph/RederGraphResource.h"
#include "Igniter/Render/RenderGraph/RenderPass.h"

namespace ig
{
    class RenderContext;
}

namespace ig::experimental
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

        GpuSync Execute();

        RGBuffer GetBuffer(const RGResourceHandle handle);
        RGTexture GetTexture(const RGResourceHandle handle);

        std::string Dump() const { return taskflow.dump(); }

    private:
        RenderContext& renderContext;

        eastl::vector<Ptr<RenderPass>> renderPasses{};
        eastl::vector<details::RGDependencyLevel> dependencyLevels{};
        eastl::vector<details::RGSyncPoint> syncPoints{};

        eastl::vector<details::RGResourceInstance> resourceInstances{};

        tf::Executor executor{std::thread::hardware_concurrency()};
        tf::Taskflow taskflow{"Render Graph"};

        eastl::vector<eastl::vector<CommandContext*>> mainGfxPendingCmdCtxLists{};
        eastl::vector<eastl::vector<CommandContext*>> asyncComputePendingCmdCtxLists{};
        eastl::vector<eastl::vector<CommandContext*>> asyncCopyPendingCmdCtxLists{};

        GpuSync lastFrameSync;
    };
}    // namespace ig::experimental