#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Render/RenderGraphBuilder.h"

IG_DEFINE_LOG_CATEGORY(RenderGraphBuilder);

namespace ig
{
    RGResourceHandle RenderGraphBuilder::CreateTexture(const GpuTextureDesc& desc)
    {
        IG_CHECK(desc.InitialLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
        const RGResourceHandle newHandle{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(details::RGResource{.Type = details::ERGResourceType::Texture, .Resource = desc, .InitialLayout = desc.InitialLayout});
        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::CloneTexture(const RGResourceHandle targetTex)
    {
        IG_CHECK(targetTex.Index < resources.size());
        const details::RGResource& resource = resources[targetTex.Index];
        IG_CHECK(resource.Type == details::ERGResourceType::Texture);
        const RGResourceHandle newHandle{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(resource);
        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::CreateBuffer(const GpuBufferDesc& desc)
    {
        const RGResourceHandle newHande{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(details::RGResource{.Type = details::ERGResourceType::Buffer, .Resource = desc});
        return newHande;
    }

    RGResourceHandle RenderGraphBuilder::CloneBuffer(const RGResourceHandle targetBuf)
    {
        IG_CHECK(targetBuf.Index < resources.size());
        const details::RGResource& resource = resources[targetBuf.Index];
        IG_CHECK(resource.Type == details::ERGResourceType::Buffer);
        const RGResourceHandle newHandle{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(resource);
        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::RegisterExternalTexture(const RGTexture texture, const D3D12_BARRIER_LAYOUT currentLayout)
    {
        IG_CHECK(currentLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
        const RGResourceHandle newHande{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(
            details::RGResource{.Type = details::ERGResourceType::Texture, .bIsExternal = true, .Resource = texture, .InitialLayout = currentLayout});
        return newHande;
    }

    RGResourceHandle RenderGraphBuilder::RegisterExternalBuffer(const RGBuffer buffer)
    {
        const RGResourceHandle newHande{.Index = static_cast<uint16_t>(resources.size())};
        resources.emplace_back(details::RGResource{.Type = details::ERGResourceType::Buffer, .bIsExternal = true, .Resource = buffer});
        return newHande;
    }

    RGResourceHandle RenderGraphBuilder::WriteTexture(
        const RGResourceHandle targetTex, const D3D12_BARRIER_LAYOUT targetLayout, const eastl::vector<RGTextureSubresouce>& subresources)
    {
        IG_CHECK(targetLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
        IG_CHECK(targetTex.Index < resources.size());
        IG_CHECK(resources[targetTex.Index].Type == details::ERGResourceType::Texture);
        IG_CHECK(!subresources.empty());
        IG_CHECK(targetTex.Version > 0 || (targetTex.Version == 0 && targetLayout == resources[targetTex.Index].InitialLayout));

        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();
        uint32_t mipLevels = 1;
        uint32_t arraySize = 0;
        details::RGResource& resource = resources[targetTex.Index];
        if (resource.bIsExternal)
        {
            const GpuTextureDesc& desc = resource.GetTextureDesc(renderContext);
            mipLevels = desc.MipLevels;
            arraySize = desc.GetArraySize();
        }
        else
        {
            const GpuTextureDesc& desc = std::get<GpuTextureDesc>(resource.Resource);
            mipLevels = desc.MipLevels;
            arraySize = desc.GetArraySize();
        }

        RGResourceHandle handle{.Index = targetTex.Index, .Version = static_cast<uint16_t>(targetTex.Version)};
        RGResourceHandle newHandle{.Index = targetTex.Index, .Version = static_cast<uint16_t>(targetTex.Version + 1)};
        for (const RGTextureSubresouce subresource : subresources)
        {
            IG_CHECK(subresource.MipSlice < mipLevels);
            IG_CHECK(subresource.ArraySlice < arraySize || (subresource.ArraySlice == 0 && arraySize == 0));
            newHandle.Subresource = handle.Subresource = D3D12CalcSubresource(subresource.MipSlice, subresource.ArraySlice, 0, mipLevels, arraySize);

            const size_t renderPassIdx = renderPasses.size() - 1;
            resourceDependencies[handle].Readers.emplace(renderPassIdx);
            // -> Equivalent with Owners.empty() && Writer == None
            IG_CHECK(!resourceDependencies.contains(newHandle) && "Duplicated Write to same version of resource!");
            resourceDependencies[newHandle].Writer = renderPassIdx;
            resourceDependencies[newHandle].Layout = targetLayout;
            renderPassPackage.WriteDependencies.emplace(newHandle);
        }

        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::WriteBuffer(const RGResourceHandle targetBuf)
    {
        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();
        IG_CHECK(targetBuf.Index < resources.size());
        IG_CHECK(resources[targetBuf.Index].Type == details::ERGResourceType::Buffer);

        const RGResourceHandle newHandle{
            .Index = targetBuf.Index, .Version = static_cast<uint16_t>(targetBuf.Version + 1), .Subresource = targetBuf.Subresource};

        const size_t renderPassIdx = renderPasses.size() - 1;
        resourceDependencies[targetBuf].Readers.insert(renderPassIdx);
        // -> Equivalent with Owners.empty() && Writer == None
        IG_CHECK(!resourceDependencies.contains(newHandle) && "Duplicated Write to same version of resource!");
        resourceDependencies[newHandle].Writer = renderPassIdx;
        renderPassPackage.WriteDependencies.emplace(newHandle);

        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::ReadTexture(
        const RGResourceHandle targetTex, const D3D12_BARRIER_LAYOUT targetLayout, const eastl::vector<RGTextureSubresouce>& subresources)
    {
        IG_CHECK(targetLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
        IG_CHECK(targetTex.Index < resources.size());
        IG_CHECK(resources[targetTex.Index].Type == details::ERGResourceType::Texture);
        IG_CHECK(subresources.size() > 0);
        IG_CHECK(targetTex.Version > 0 || (targetTex.Version == 0 && targetLayout == resources[targetTex.Index].InitialLayout));

        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();

        uint32_t mipLevels = 1;
        uint32_t arraySize = 0;
        details::RGResource& resource = resources[targetTex.Index];
        if (resource.bIsExternal)
        {
            const GpuTextureDesc& desc = resource.GetTextureDesc(renderContext);
            mipLevels = desc.MipLevels;
            arraySize = desc.GetArraySize();
            renderPassPackage.bHasNonExternalReadDependency = true;
        }
        else
        {
            const GpuTextureDesc& desc = std::get<GpuTextureDesc>(resource.Resource);
            mipLevels = desc.MipLevels;
            arraySize = desc.GetArraySize();
        }

        RGResourceHandle handle{.Index = targetTex.Index, .Version = targetTex.Version};
        RGResourceHandle newHandle{.Index = targetTex.Index, .Version = static_cast<uint16_t>(targetTex.Version + 1)};
        for (const RGTextureSubresouce subresource : subresources)
        {
            IG_CHECK(subresource.MipSlice < mipLevels);
            IG_CHECK(subresource.ArraySlice < arraySize || (subresource.ArraySlice == 0 && arraySize == 0));
            newHandle.Subresource = handle.Subresource = D3D12CalcSubresource(subresource.MipSlice, subresource.ArraySlice, 0, mipLevels, arraySize);

            const size_t renderPassIdx = renderPasses.size() - 1;
            resourceDependencies[handle].Readers.emplace(renderPassIdx);
            IG_CHECK(!resourceDependencies[newHandle].HasWriter());
            resourceDependencies[newHandle].Owners.emplace(renderPassIdx);
            IG_CHECK(
                resourceDependencies[newHandle].Layout == D3D12_BARRIER_LAYOUT_UNDEFINED || resourceDependencies[newHandle].Layout != targetLayout);
            resourceDependencies[newHandle].Layout = targetLayout;
            renderPassPackage.ReadDependencies.emplace(newHandle);
        };

        newHandle.Subresource = 0;
        return newHandle;
    }

    RGResourceHandle RenderGraphBuilder::ReadBuffer(const RGResourceHandle targetBuf)
    {
        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();
        IG_CHECK(targetBuf.Index < resources.size());
        IG_CHECK(resources[targetBuf.Index].Type == details::ERGResourceType::Buffer);

        renderPassPackage.ReadDependencies.emplace(targetBuf);
        if (resources[targetBuf.Index].bIsExternal)
        {
            renderPassPackage.bHasNonExternalReadDependency = true;
        }

        RGResourceHandle newHandle{.Index = targetBuf.Index, .Version = static_cast<uint16_t>(targetBuf.Version + 1)};
        const size_t renderPassIdx = renderPasses.size() - 1;
        resourceDependencies[targetBuf].Readers.emplace(renderPassIdx);
        IG_CHECK(!resourceDependencies[newHandle].HasWriter());
        resourceDependencies[newHandle].Owners.emplace(renderPassIdx);

        return newHandle;
    }

    void RenderGraphBuilder::BuildAdjanceyList()
    {
        for (const auto& [handle, resourceDependency] : resourceDependencies)
        {
            for (const size_t owner : resourceDependency.Owners)
            {
                renderPasses[owner].AdjanceyRenderPasses.insert(resourceDependency.Readers.begin(), resourceDependency.Readers.end());
            }

            if (resourceDependency.HasWriter())
            {
                renderPasses[resourceDependency.Writer].AdjanceyRenderPasses.insert(
                    resourceDependency.Readers.begin(), resourceDependency.Readers.end());
            }
        }
    }

    void RenderGraphBuilder::BuildTopologicalOrderedRenderPassIndices()
    {
        eastl::vector<bool> visited(renderPasses.size());
        eastl::vector<bool> onStack(renderPasses.size());

        for (size_t renderPassIdx : views::iota(0Ui64, renderPasses.size()))
        {
            TopologicalSortDFS(renderPassIdx, visited, onStack);
        }

        eastl::reverse(topologicalOrderedRenderPassIndices.begin(), topologicalOrderedRenderPassIndices.end());
    }

    void RenderGraphBuilder::TopologicalSortDFS(const size_t renderPassIdx, eastl::vector<bool>& visited, eastl::vector<bool>& onStack)
    {
        IG_CHECK(!onStack[renderPassIdx] && "Found circular dependency!");
        if (!visited[renderPassIdx])
        {
            details::RGRenderPassPackage& renderPassPackage = renderPasses[renderPassIdx];

            visited[renderPassIdx] = true;
            onStack[renderPassIdx] = true;
            for (const size_t adjacentRenderPassIdx : renderPassPackage.AdjanceyRenderPasses)
            {
                TopologicalSortDFS(adjacentRenderPassIdx, visited, onStack);
            }
            onStack[renderPassIdx] = false;

            topologicalOrderedRenderPassIndices.emplace_back(renderPassIdx);
        }
    }

    void RenderGraphBuilder::CalculateDependencyLevels()
    {
        for (const size_t renderPassIdx : topologicalOrderedRenderPassIndices)
        {
            const details::RGRenderPassPackage& renderPassPackage = renderPasses[renderPassIdx];
            for (const size_t adjacentRenderPassIdx : renderPassPackage.AdjanceyRenderPasses)
            {
                details::RGRenderPassPackage& adjacentRenderPassPackage = renderPasses[adjacentRenderPassIdx];
                if (adjacentRenderPassPackage.DependencyLevel < renderPassPackage.DependencyLevel + 1)
                {
                    adjacentRenderPassPackage.DependencyLevel = renderPassPackage.DependencyLevel + 1;
                    maxDependencyLevels = std::max(maxDependencyLevels, adjacentRenderPassPackage.DependencyLevel);
                }
            }
        }
    }

    void RenderGraphBuilder::BuildDependencyLevels()
    {
        ++maxDependencyLevels;
        dependencyLevels.resize(maxDependencyLevels);
        for (const uint16_t dependencyLevelIdx : views::iota(0Ui16, maxDependencyLevels))
        {
            dependencyLevels[dependencyLevelIdx].Level = dependencyLevelIdx;
        }

        for (const size_t renderPassIdx : views::iota(0Ui64, renderPasses.size()))
        {
            const details::RGRenderPassPackage& renderPassPackage = renderPasses[renderPassIdx];
            details::RGDependencyLevel& dependencyLevel = dependencyLevels[renderPassPackage.DependencyLevel];

            switch (renderPassPackage.ExecuteOn)
            {
                case details::ERGExecutableQueue::MainGfx:
                    dependencyLevel.MainGfxQueueRenderPasses.emplace_back(renderPassIdx);
                    break;
                case details::ERGExecutableQueue::AsyncCompute:
                    dependencyLevel.AsyncComputeQueueRenderPasses.emplace_back(renderPassIdx);
                    break;
                case details::ERGExecutableQueue::AsyncCopy:
                    dependencyLevel.AsyncCopyQueueRenderPasses.emplace_back(renderPassIdx);
                    break;
            }

            dependencyLevel.RenderPasses.emplace_back(renderPassIdx);
        }
    }

    void RenderGraphBuilder::BuildSyncPoints()
    {
        // 여기서 사용할 RGResourceHandle은 Version을 무시해야 한다
        // 같은 Dependency Level에서의 다른 Layout은 현재로선 미지원
        // (대부분 그럴 경우가 없을 것 같긴한데.. 필요한 경우 더 generic 한 layout으로 사용 할 것..)
        // 예시. Shader Resource 이면서 Copy_src 인 경우 => READ_GENERIC
        UnorderedMap<RGResourceHandle, D3D12_BARRIER_LAYOUT> layoutTable{};
        for (const auto& [handle, dependency] : resourceDependencies)
        {
            const details::RGResource& resource = resources[handle.Index];
            if (resource.Type == details::ERGResourceType::Texture)
            {
                IG_CHECK(resource.InitialLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
                layoutTable[handle.MakeVersionLess()] = resource.InitialLayout;
            }
        }

        for (size_t dependencyLevelIdx : views::iota(1Ui64, dependencyLevels.size()))
        {
            details::RGSyncPoint& syncPoint = syncPoints.emplace_back();
            syncPoint.bSyncWithAsyncComputeQueue = !dependencyLevels[dependencyLevelIdx - 1].AsyncComputeQueueRenderPasses.empty();
            syncPoint.bSyncWithAsyncCopyQueue = !dependencyLevels[dependencyLevelIdx - 1].AsyncCopyQueueRenderPasses.empty();

            for (const size_t renderPassIdx : dependencyLevels[dependencyLevelIdx].RenderPasses)
            {
                for (const RGResourceHandle handle : renderPasses[renderPassIdx].ReadDependencies)
                {
                    if (resources[handle.Index].Type == details::ERGResourceType::Buffer)
                    {
                        continue;
                    }

                    const D3D12_BARRIER_LAYOUT after = resourceDependencies[handle].Layout;
                    IG_CHECK(after != D3D12_BARRIER_LAYOUT_UNDEFINED);

                    const RGResourceHandle versionLessHandle = handle.MakeVersionLess();
                    const D3D12_BARRIER_LAYOUT before = layoutTable[versionLessHandle];
                    if (before != after)
                    {
                        syncPoint.LayoutTransitions.emplace_back(details::RGLayoutTransition{
                            .TextureHandle = versionLessHandle, .Before = layoutTable[versionLessHandle], .After = after});
                        layoutTable[versionLessHandle] = after;
                    }
                }

                for (const RGResourceHandle handle : renderPasses[renderPassIdx].WriteDependencies)
                {
                    if (resources[handle.Index].Type == details::ERGResourceType::Buffer)
                    {
                        continue;
                    }

                    const D3D12_BARRIER_LAYOUT after = resourceDependencies[handle].Layout;
                    if (after == D3D12_BARRIER_LAYOUT_UNDEFINED)
                        continue;
                    IG_CHECK(after != D3D12_BARRIER_LAYOUT_UNDEFINED);

                    const RGResourceHandle versionLessHandle = handle.MakeVersionLess();
                    const D3D12_BARRIER_LAYOUT before = layoutTable[versionLessHandle];
                    if (before != after)
                    {
                        syncPoint.LayoutTransitions.emplace_back(details::RGLayoutTransition{
                            .TextureHandle = versionLessHandle, .Before = layoutTable[versionLessHandle], .After = after});
                        layoutTable[versionLessHandle] = after;
                    }
                }
            }
        }

        // 마지막 단계 -> 초기 상태
        // Resource Desc의 Initial Layout을 참고해서 초기화
        details::RGSyncPoint& lastSyncPoint = syncPoints.emplace_back();
        for (const auto& [handle, lastLayout] : layoutTable)
        {
            const details::RGResource& resource = resources[handle.Index];
            IG_CHECK(resource.Type == details::ERGResourceType::Texture);

            const D3D12_BARRIER_LAYOUT initialLayout = resource.InitialLayout;
            if (lastLayout != initialLayout)
            {
                lastSyncPoint.LayoutTransitions.emplace_back(
                    details::RGLayoutTransition{.TextureHandle = handle.MakeVersionLess(), .Before = lastLayout, .After = initialLayout});
            }
        }
    }

    Ptr<RenderGraph> RenderGraphBuilder::Compile()
    {
        BuildAdjanceyList();
        for (const auto& renderPassPackage : renderPasses)
        {
            std::cout << std::format("{}: ", renderPassPackage.RenderPassPtr->GetName());
            for (const auto& adjacentPassIdx : renderPassPackage.AdjanceyRenderPasses)
            {
                std::cout << std::format("{}, ", renderPasses[adjacentPassIdx].RenderPassPtr->GetName());
            }
            std::cout << '\n';
        }
        BuildTopologicalOrderedRenderPassIndices();
        std::cout << "* Topological Sorted" << std::endl;
        for (const auto& renderPassIdx : topologicalOrderedRenderPassIndices)
        {
            const auto& renderPassPackage = renderPasses[renderPassIdx];
            std::cout << std::format("{}, ", renderPassPackage.RenderPassPtr->GetName());
        }
        std::cout << '\n';

        CalculateDependencyLevels();
        std::cout << "* Dependency Levels" << '\n';
        for (const auto& renderPassPackage : renderPasses)
        {
            std::cout << std::format("{}: {}", renderPassPackage.RenderPassPtr->GetName(), renderPassPackage.DependencyLevel) << '\n';
        }

        BuildDependencyLevels();
        BuildSyncPoints();
        return MakePtr<RenderGraph>(*this);
    }

}    // namespace ig