#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Render/RenderGraph/RenderGraphBuilder.h"

IG_DEFINE_LOG_CATEGORY(RenderGraphBuilder);

namespace ig::experimental
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

        RGResourceHandle subresourceHandleOrigin{.Index = targetTex.Index, .Version = static_cast<uint16_t>(targetTex.Version)};
        RGResourceHandle subresourceHandle{.Index = targetTex.Index, .Version = static_cast<uint16_t>(targetTex.Version + 1)};
        for (const RGTextureSubresouce subresource : subresources)
        {
            IG_CHECK(subresource.MipSlice < mipLevels);
            IG_CHECK(subresource.ArraySlice < arraySize || (subresource.ArraySlice == 0 && arraySize == 0));
            subresourceHandle.Subresource = subresourceHandleOrigin.Subresource =
                D3D12CalcSubresource(subresource.MipSlice, subresource.ArraySlice, 0, mipLevels, arraySize);

            const size_t renderPassIdx = renderPasses.size() - 1;
            resourceDependencies[subresourceHandleOrigin].Readers.insert(renderPassIdx);
            renderPassPackage.WriteDependencies[subresourceHandle] = targetLayout;
            resourceDependencies[subresourceHandle].Writer = renderPassIdx;
        }

        return subresourceHandle;
    }

    RGResourceHandle RenderGraphBuilder::WriteBuffer(const RGResourceHandle targetBuf)
    {
        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();
        IG_CHECK(targetBuf.Index < resources.size());
        IG_CHECK(resources[targetBuf.Index].Type == details::ERGResourceType::Buffer);

        const RGResourceHandle newVersionHandle{
            .Index = targetBuf.Index, .Version = static_cast<uint16_t>(targetBuf.Version + 1), .Subresource = targetBuf.Subresource};

        const size_t renderPassIdx = renderPasses.size() - 1;
        resourceDependencies[targetBuf].Readers.insert(renderPassIdx);
        renderPassPackage.WriteDependencies[newVersionHandle] = D3D12_BARRIER_LAYOUT_UNDEFINED;
        resourceDependencies[newVersionHandle].Writer = renderPassIdx;

        return newVersionHandle;
    }

    void RenderGraphBuilder::ReadTexture(
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

        RGResourceHandle subresourceHandle{.Index = targetTex.Index, .Version = targetTex.Version};
        for (const RGTextureSubresouce subresource : subresources)
        {
            IG_CHECK(subresource.MipSlice < mipLevels);
            IG_CHECK(subresource.ArraySlice < arraySize || (subresource.ArraySlice == 0 && arraySize == 0));
            subresourceHandle.Subresource = D3D12CalcSubresource(subresource.MipSlice, subresource.ArraySlice, 0, mipLevels, arraySize);
            renderPassPackage.ReadDependencies[subresourceHandle] = targetLayout;
            IG_CHECK(resourceDependencies.contains(subresourceHandle));
            resourceDependencies[subresourceHandle].Readers.emplace(renderPasses.size() - 1);
        };
    }

    void RenderGraphBuilder::ReadBuffer(const RGResourceHandle targetBuf)
    {
        details::RGRenderPassPackage& renderPassPackage = renderPasses.back();
        IG_CHECK(targetBuf.Index < resources.size());
        IG_CHECK(resources[targetBuf.Index].Type == details::ERGResourceType::Buffer);
        renderPassPackage.ReadDependencies[targetBuf] = D3D12_BARRIER_LAYOUT_UNDEFINED;
        if (resources[targetBuf.Index].bIsExternal)
        {
            renderPassPackage.bHasNonExternalReadDependency = true;
        }

        IG_CHECK(resourceDependencies.contains(targetBuf));
        resourceDependencies[targetBuf].Readers.emplace(renderPasses.size() - 1);
    }

    void RenderGraphBuilder::ResolveDependencies()
    {
        eastl::vector<bool> visited(renderPasses.size());
        eastl::vector<bool> onStack(renderPasses.size());

        for (size_t renderPassIdx : views::iota(0Ui64, renderPasses.size()))
        {
            details::RGRenderPassPackage& renderPassPackage = renderPasses[renderPassIdx];
            if (!renderPassPackage.bHasNonExternalReadDependency || renderPassPackage.ReadDependencies.empty())
            {
                DFS(renderPassIdx, 0, visited, onStack);
            }
        }
    }

    void RenderGraphBuilder::DFS(const size_t renderPassIdx, const uint16_t depth, eastl::vector<bool>& visited, eastl::vector<bool>& onStack)
    {
        IG_CHECK(!onStack[renderPassIdx] && "Found circular dependency!");
        maxDependencyLevels = std::max(maxDependencyLevels, depth);
        if (!visited[renderPassIdx] || renderPasses[renderPassIdx].DependencyLevel < depth)
        {
            details::RGRenderPassPackage& renderPassPackage = renderPasses[renderPassIdx];
            UnorderedSet<size_t> readDependentRenderPassIndices;
            for (const auto& [writeResourceHandle, layout] : renderPassPackage.WriteDependencies)
            {
                const ResourceDependency& dependencyInfo = resourceDependencies[writeResourceHandle];
                IG_CHECK(dependencyInfo.Writer == renderPassIdx);
                readDependentRenderPassIndices.insert(dependencyInfo.Readers.cbegin(), dependencyInfo.Readers.cend());
            }

            visited[renderPassIdx] = true;
            onStack[renderPassIdx] = true;
            for (const size_t readDependentRenderPassIdx : readDependentRenderPassIndices)
            {
                DFS(readDependentRenderPassIdx, depth + 1, visited, onStack);
            }
            onStack[renderPassIdx] = false;

            renderPasses[renderPassIdx].DependencyLevel = depth;
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

        // DL#0 ### SP#0 ### DL#1 ### SP1 ### DL#2 ### SP-LAST
        // Layout Table의 값과 비교해서 '다르다면' Layout Transition 추가
        // 같으면 무시
        // 만약 새로운 값이라면 Layout Table에 새롭게 추가만하기!
        for (size_t dependencyLevelIdx : views::iota(1Ui64, dependencyLevels.size()))
        {
            details::RGSyncPoint& syncPoint = syncPoints.emplace_back();
            syncPoint.bSyncWithAsyncComputeQueue = !dependencyLevels[dependencyLevelIdx - 1].AsyncComputeQueueRenderPasses.empty();
            syncPoint.bSyncWithAsyncCopyQueue = !dependencyLevels[dependencyLevelIdx - 1].AsyncCopyQueueRenderPasses.empty();

            for (const size_t renderPassIdx : dependencyLevels[dependencyLevelIdx].RenderPasses)
            {
                for (const auto& [handle, layout] : renderPasses[renderPassIdx].ReadDependencies)
                {
                    if (layout == D3D12_BARRIER_LAYOUT_UNDEFINED)
                    {
                        continue;
                    }

                    const RGResourceHandle versionLessHandle = handle.MakeVersionLess();
                    const D3D12_BARRIER_LAYOUT before = layoutTable[versionLessHandle];
                    if (before != layout)
                    {
                        syncPoint.LayoutTransitions.emplace_back(details::RGLayoutTransition{
                            .TextureHandle = versionLessHandle, .Before = layoutTable[versionLessHandle], .After = layout});
                        layoutTable[versionLessHandle] = layout;
                    }
                }

                for (const auto& [handle, layout] : renderPasses[renderPassIdx].WriteDependencies)
                {
                    if (layout == D3D12_BARRIER_LAYOUT_UNDEFINED)
                    {
                        continue;
                    }

                    const RGResourceHandle versionLessHandle = handle.MakeVersionLess();
                    const D3D12_BARRIER_LAYOUT before = layoutTable[versionLessHandle];
                    if (before != layout)
                    {
                        syncPoint.LayoutTransitions.emplace_back(details::RGLayoutTransition{
                            .TextureHandle = versionLessHandle, .Before = layoutTable[versionLessHandle], .After = layout});
                        layoutTable[versionLessHandle] = layout;
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
        ResolveDependencies();
        BuildDependencyLevels();
        BuildSyncPoints();
        return MakePtr<RenderGraph>(*this);
    }
}    // namespace ig::experimental