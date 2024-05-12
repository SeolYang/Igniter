#include "Igniter/Igniter.h"
#include "Igniter/Core/ContainerUtils.h"
#include "Igniter/Render/RenderGraphBuilder.h"
#include "Igniter/Render/RenderGraph.h"

namespace ig
{
    RenderGraph::RenderGraph(RenderGraphBuilder& builder) : renderContext(builder.renderContext)
    {
        renderPasses.resize(builder.renderPasses.size());
        std::transform(builder.renderPasses.begin(), builder.renderPasses.end(), renderPasses.begin(),
            [](details::RGRenderPassPackage& renderPassPackage) { return std::move(renderPassPackage.RenderPassPtr); });

        dependencyLevels = std::move(builder.dependencyLevels);
        syncPoints = std::move(builder.syncPoints);
        IG_CHECK(dependencyLevels.size() == syncPoints.size());

        resourceInstances.reserve(builder.resources.size());
        for (const auto& resource : builder.resources)
        {
            /* #sy_todo 실제로 사용되지 않고있는 (No Writer/Readers) 자원의 경우 무시 */
            if (resource.Type == details::ERGResourceType::Buffer)
            {
                RGBuffer newBuffer{};
                if (resource.bIsExternal)
                {
                    newBuffer = std::get<RGBuffer>(resource.Resource);
                }
                else
                {
                    const GpuBufferDesc& desc = resource.GetBufferDesc(renderContext);
                    for (auto& handle : newBuffer.Resources)
                    {
                        handle = renderContext.CreateBuffer(desc);
                    }
                }

                resourceInstances.emplace_back(details::RGResourceInstance{
                    .Type = details::ERGResourceType::Buffer, .Resource = newBuffer, .bIsExternal = resource.bIsExternal});
            }
            else
            {
                RGTexture newTexture{};
                if (resource.bIsExternal)
                {
                    newTexture = std::get<RGTexture>(resource.Resource);
                }
                else
                {
                    const GpuTextureDesc& desc = resource.GetTextureDesc(renderContext);
                    for (auto& handle : newTexture.Resources)
                    {
                        handle = renderContext.CreateTexture(desc);
                    }
                }

                resourceInstances.emplace_back(details::RGResourceInstance{
                    .Type = details::ERGResourceType::Texture, .Resource = newTexture, .bIsExternal = resource.bIsExternal});
            }
        }

        for (auto& renderPass : renderPasses)
        {
            renderPass->PostCompile(*this);
        }

        /******** 렌더 태스크 그래프 생성 *********/
        const tf::Task renderBeginTask = taskflow.placeholder().name("Render Task:Begin");
        tf::Task prevSyncTask = renderBeginTask;
        for (const size_t dependencyLevelIdx : views::iota(0Ui64, dependencyLevels.size()))
        {
            const details::RGDependencyLevel& dependencyLevel = dependencyLevels[dependencyLevelIdx];

            const String syncPointName{std::format("Sync DL#{} -> DL#{}", dependencyLevelIdx, (dependencyLevelIdx + 1) % dependencyLevels.size())};
            tf::Task syncTask = taskflow.placeholder().name(syncPointName.ToStandard());

            // QueueTask (vector<vector<Cmd Ctx>>) <- Render Pass (vector<Cmd Ctx>) <- Sub Render Pass (Cmd Ctx)
            if (!dependencyLevel.MainGfxQueueRenderPasses.empty())
            {
                tf::Task mainGfxTask = taskflow.placeholder().name(std::format("DL#{}: Main Gfx", dependencyLevelIdx));
                mainGfxTask.succeed(prevSyncTask);
                tf::Task mainGfxSubmitTask = taskflow.placeholder().name(std::format("DL#{}: Main Gfx Submit", dependencyLevelIdx));

                mainGfxPendingCmdCtxLists.resize(std::max(mainGfxPendingCmdCtxLists.size(), dependencyLevel.MainGfxQueueRenderPasses.size()));
                size_t localRenderPassIdx = 0;    // 현재 Dependency Level의 Main Gfx Queue Render Passes 내에서의 인덱스
                for (const size_t renderPassIdx : dependencyLevel.MainGfxQueueRenderPasses)
                {
                    RenderPass& renderPass = *renderPasses[renderPassIdx];
                    tf::Task renderPassTask = taskflow
                                                  .emplace(
                                                      [this, &renderPass, localRenderPassIdx](tf::Subflow& renderPassSubflow)
                                                      {
                                                          mainGfxPendingCmdCtxLists[localRenderPassIdx].clear();
                                                          renderPass.Execute(renderPassSubflow, mainGfxPendingCmdCtxLists[localRenderPassIdx]);
                                                      })
                                                  .name(std::format("DL#{}: {}", dependencyLevelIdx, renderPass.GetName()));
                    renderPassTask.succeed(mainGfxTask);
                    mainGfxSubmitTask.succeed(renderPassTask);

                    ++localRenderPassIdx;
                }

                mainGfxSubmitTask.work(
                    [this]()
                    {
                        eastl::vector<CommandContext*> pendingCmdCtxList = ToVector(views::join(mainGfxPendingCmdCtxLists));
                        renderContext.GetMainGfxQueue().ExecuteContexts(pendingCmdCtxList);
                    });
                syncTask.succeed(mainGfxSubmitTask);
            }

            if (!dependencyLevel.AsyncComputeQueueRenderPasses.empty())
            {
                tf::Task asyncComputeTask = taskflow.placeholder().name(std::format("DL#{}: Async Compute", dependencyLevelIdx));
                asyncComputeTask.succeed(prevSyncTask);
                tf::Task asyncComputeSubmitTask = taskflow.placeholder().name(std::format("DL#{}: Async Compute Submit", dependencyLevelIdx));

                asyncComputePendingCmdCtxLists.resize(
                    std::max(asyncComputePendingCmdCtxLists.size(), dependencyLevel.AsyncComputeQueueRenderPasses.size()));
                size_t localRenderPassIdx = 0;    // 현재 Dependency Level의 Async Compute Queue Render Passes 내에서의 인덱스
                for (const size_t renderPassIdx : dependencyLevel.AsyncComputeQueueRenderPasses)
                {
                    RenderPass& renderPass = *renderPasses[renderPassIdx];
                    tf::Task renderPassTask = taskflow
                                                  .emplace(
                                                      [this, &renderPass, localRenderPassIdx](tf::Subflow& renderPassSubflow)
                                                      {
                                                          asyncComputePendingCmdCtxLists[localRenderPassIdx].clear();
                                                          renderPass.Execute(renderPassSubflow, asyncComputePendingCmdCtxLists[localRenderPassIdx]);
                                                      })
                                                  .name(std::format("DL#{}: {}", dependencyLevelIdx, renderPass.GetName()));
                    renderPassTask.succeed(asyncComputeTask);
                    asyncComputeSubmitTask.succeed(renderPassTask);

                    ++localRenderPassIdx;
                }

                asyncComputeSubmitTask.work(
                    [this, renderBeginTask, prevSyncTask]()
                    {
                        CommandQueue& asyncComputeQueue = renderContext.GetAsyncComputeQueue();
                        eastl::vector<CommandContext*> pendingCmdCtxList = ToVector(views::join(asyncComputePendingCmdCtxLists));
                        if (renderBeginTask != prevSyncTask)
                        {
                            // #sy_note 이미 이전 직전 Sync Point에 의해 Main Gfx Queue에서 큐 간 동기화가 일어나기 때문에 그대로 사용
                            GpuSync prevSyncPoint = renderContext.GetMainGfxQueue().GetSync();
                            asyncComputeQueue.SyncWith(prevSyncPoint);
                        }
                        asyncComputeQueue.ExecuteContexts(pendingCmdCtxList);
                        asyncComputeQueue.MakeSync();
                    });
                syncTask.succeed(asyncComputeSubmitTask);
            }

            if (!dependencyLevel.AsyncCopyQueueRenderPasses.empty())
            {
                tf::Task asyncCopyTask = taskflow.placeholder().name(std::format("DL#{}: Async Copy", dependencyLevelIdx));
                asyncCopyTask.succeed(prevSyncTask);
                tf::Task asyncCopySubmitTask = taskflow.placeholder().name(std::format("DL#{}: Async Copy Submit", dependencyLevelIdx));

                asyncCopyPendingCmdCtxLists.resize(std::max(asyncCopyPendingCmdCtxLists.size(), dependencyLevel.AsyncCopyQueueRenderPasses.size()));
                size_t localRenderPassIdx = 0;    // 현재 Dependency Level의 Async Copy Queue Render Passes 내에서의 인덱스
                for (const size_t renderPassIdx : dependencyLevel.AsyncCopyQueueRenderPasses)
                {
                    RenderPass& renderPass = *renderPasses[renderPassIdx];
                    tf::Task renderPassTask = taskflow
                                                  .emplace(
                                                      [this, &renderPass, localRenderPassIdx](tf::Subflow& renderPassSubflow)
                                                      {
                                                          asyncCopyPendingCmdCtxLists[localRenderPassIdx].clear();
                                                          renderPass.Execute(renderPassSubflow, asyncCopyPendingCmdCtxLists[localRenderPassIdx]);
                                                      })
                                                  .name(std::format("DL#{}: {}", dependencyLevelIdx, renderPass.GetName()));
                    renderPassTask.succeed(asyncCopyTask);
                    asyncCopySubmitTask.succeed(renderPassTask);

                    ++localRenderPassIdx;
                }

                asyncCopySubmitTask.work(
                    [this, renderBeginTask, prevSyncTask]()
                    {
                        CommandQueue& asyncCopyQueue = renderContext.GetAsyncCopyQueue();
                        eastl::vector<CommandContext*> pendingCmdCtxList = ToVector(views::join(asyncCopyPendingCmdCtxLists));
                        if (renderBeginTask != prevSyncTask)
                        {
                            GpuSync prevSyncPoint = renderContext.GetMainGfxQueue().GetSync();
                            asyncCopyQueue.SyncWith(prevSyncPoint);
                        }

                        asyncCopyQueue.ExecuteContexts(pendingCmdCtxList);
                        asyncCopyQueue.MakeSync();
                    });
                syncTask.succeed(asyncCopySubmitTask);
            }

            syncTask.work(
                [this, syncPointIdx = dependencyLevelIdx, syncPointName]()
                {
                    CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();
                    details::RGSyncPoint& syncPoint = syncPoints[syncPointIdx];
                    if (syncPoint.bSyncWithAsyncComputeQueue)
                    {
                        GpuSync asyncComputeSync = renderContext.GetAsyncComputeQueue().GetSync();
                        mainGfxQueue.SyncWith(asyncComputeSync);
                    }
                    if (syncPoint.bSyncWithAsyncCopyQueue)
                    {
                        GpuSync asyncCopySync = renderContext.GetAsyncCopyQueue().GetSync();
                        mainGfxQueue.SyncWith(asyncCopySync);
                    }

                    // #sy_note Layout Transitions이 필요 없는 경우 단순히 큐 간의 동기화만 보장 하도록
                    const LocalFrameIndex localFrameIdx = FrameManager::GetLocalFrameIndex();
                    auto layoutTransitionCmdCtx =
                        renderContext.GetMainGfxCommandContextPool().Request(FrameManager::GetLocalFrameIndex(), syncPointName);
                    layoutTransitionCmdCtx->Begin();
                    /* #sy_todo 미리 RenderGraph에 만들어 놓기? */
                    for (details::RGLayoutTransition layoutTransition : syncPoint.LayoutTransitions)
                    {
                        IG_CHECK(layoutTransition.Before != layoutTransition.After);

                        RGTexture rgTexture = resourceInstances[layoutTransition.TextureHandle.Index].GetTexture();
                        GpuTexture* gpuTexturePtr = renderContext.Lookup(rgTexture.Resources[localFrameIdx]);
                        IG_CHECK(gpuTexturePtr != nullptr);
                        layoutTransitionCmdCtx->AddPendingTextureBarrier(*gpuTexturePtr, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS, layoutTransition.Before, layoutTransition.After,
                            D3D12_BARRIER_SUBRESOURCE_RANGE{.IndexOrFirstMipLevel = layoutTransition.TextureHandle.Subresource});
                    }
                    layoutTransitionCmdCtx->FlushBarriers();
                    layoutTransitionCmdCtx->End();

                    CommandContext* cmdCtxs[1]{(CommandContext*) layoutTransitionCmdCtx};
                    mainGfxQueue.ExecuteContexts(cmdCtxs);
                    mainGfxQueue.MakeSync();
                });

            prevSyncTask = syncTask;
        }

        tf::Task renderEndTask = taskflow.emplace([this]() { lastFrameSync = renderContext.GetMainGfxQueue().GetSync(); }).name("Render Task:End");
        renderEndTask.succeed(prevSyncTask);
    }

    RenderGraph::~RenderGraph()
    {
        /* #sy_todo External Resource가 아니면 해제 */
        for (details::RGResourceInstance& resourceInstance : resourceInstances)
        {
            if (resourceInstance.bIsExternal)
            {
                continue;
            }

            switch (resourceInstance.Type)
            {
                case details::ERGResourceType::Buffer:
                    for (const auto handle : resourceInstance.GetBuffer().Resources)
                    {
                        renderContext.DestroyBuffer(handle);
                    }
                    break;

                case details::ERGResourceType::Texture:
                    for (const auto handle : resourceInstance.GetTexture().Resources)
                    {
                        renderContext.DestroyTexture(handle);
                    }
                    break;
            }
        }
    }

    GpuSync RenderGraph::Execute(tf::Executor& taskExecutor)
    {
        tf::Future<void> execution = taskExecutor.run(taskflow);
        execution.wait();
        return lastFrameSync;
    }

    RGBuffer RenderGraph::GetBuffer(const RGResourceHandle handle)
    {
        if (handle.Index >= resourceInstances.size() || resourceInstances[handle.Index].Type != details::ERGResourceType::Buffer)
        {
            return {};
        }

        return std::get<RGBuffer>(resourceInstances[handle.Index].Resource);
    }

    RGTexture RenderGraph::GetTexture(const RGResourceHandle handle)
    {
        if (handle.Index >= resourceInstances.size() || resourceInstances[handle.Index].Type != details::ERGResourceType::Texture)
        {
            return {};
        }

        return std::get<RGTexture>(resourceInstances[handle.Index].Resource);
    }
}    // namespace ig