#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/GpuSyncPoint.h"
#include "Igniter/Render/Light.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/CommandListPool.h"
#include "Igniter/Render/RenderPass/LightClusteringPass.h"

namespace ig
{
    LightClusteringPass::LightClusteringPass(RenderContext& renderContext, const SceneProxy& sceneProxy, const Viewport& mainViewport)
        : renderContext(&renderContext)
        , sceneProxy(&sceneProxy)
    {
        lightProxyMapIdxList.reserve(kMaxNumLights);

        GpuBufferDesc lightIdxListStagingBufferDesc{};
        lightIdxListStagingBufferDesc.AsUploadBuffer((U32)sizeof(U32) * kMaxNumLights);

        GpuBufferDesc lightIdxListBufferDesc{};
        lightIdxListBufferDesc.AsStructuredBuffer<U32>(kMaxNumLights, false, false);

        GpuBufferDesc tilesBufferDesc{};
        GpuBufferDesc depthBinsBufferDesc{};

        numTileX = (Size)mainViewport.width / kTileSize;
        numTileY = (Size)mainViewport.height / kTileSize;

        tileDwordsStride = kNumDWordsPerTile * numTileX;
        numTileDwords = tileDwordsStride * numTileY;
        tilesBufferDesc.AsUploadBuffer((U32)(numTileDwords * sizeof(U32)), true);
        depthBinsBufferDesc.AsUploadBuffer(kDepthBinsBufferSize, true);

        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            lightIdxListStagingBufferDesc.DebugName = String(std::format("LightIdxListStaging.{}", localFrameIdx));
            lightIdxListBufferDesc.DebugName = String(std::format("LightIdxListBuffer.{}", localFrameIdx));

            const RenderHandle<GpuBuffer> newLightIdxListStagingBuffer = renderContext.CreateBuffer(lightIdxListStagingBufferDesc);
            lightIdxListStagingBuffer[localFrameIdx] = newLightIdxListStagingBuffer;
            GpuBuffer* newLightIdxListStagingBufferPtr = renderContext.Lookup(newLightIdxListStagingBuffer);
            mappedLightIdxListStagingBuffer[localFrameIdx] = reinterpret_cast<U32*>(newLightIdxListStagingBufferPtr->Map());

            const RenderHandle<GpuBuffer> newLightIdxListBuffer = renderContext.CreateBuffer(lightIdxListBufferDesc);
            lightIdxListBufferPackage[localFrameIdx].Buffer = newLightIdxListBuffer;
            lightIdxListBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newLightIdxListBuffer);

            tilesBufferDesc.DebugName = String(std::format("TilesBuffer.{}", localFrameIdx));
            depthBinsBufferDesc.DebugName = String(std::format("DepthBinsBuffer.{}", localFrameIdx));

            const RenderHandle<GpuBuffer> newTilesBuffer = renderContext.CreateBuffer(tilesBufferDesc);
            tilesBufferPackage[localFrameIdx].Buffer = newTilesBuffer;
            tilesBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newTilesBuffer);
            GpuBuffer* newTilesBufferPtr = renderContext.Lookup(newTilesBuffer);
            IG_CHECK(newTilesBufferPtr != nullptr);
            tilesBufferPackage[localFrameIdx].MappedBuffer = newTilesBufferPtr->Map();

            const RenderHandle<GpuBuffer> newDepthBinsBuffer = renderContext.CreateBuffer(depthBinsBufferDesc);
            depthBinsBufferPackage[localFrameIdx].Buffer = newDepthBinsBuffer;
            depthBinsBufferPackage[localFrameIdx].Srv = renderContext.CreateShaderResourceView(newDepthBinsBuffer);
            GpuBuffer* newDepthBinsBufferPtr = renderContext.Lookup(newDepthBinsBuffer);
            IG_CHECK(newDepthBinsBufferPtr != nullptr);
            depthBinsBufferPackage[localFrameIdx].MappedBuffer = newDepthBinsBufferPtr->Map();
        }
    }

    LightClusteringPass::~LightClusteringPass()
    {
        for (const LocalFrameIndex localFrameIdx : LocalFramesView)
        {
            renderContext->DestroyGpuView(lightIdxListBufferPackage[localFrameIdx].Srv);
            renderContext->DestroyBuffer(lightIdxListBufferPackage[localFrameIdx].Buffer);
            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            lightIdxListStagingBufferPtr->Unmap();
            renderContext->DestroyBuffer(lightIdxListStagingBuffer[localFrameIdx]);

            renderContext->DestroyGpuView(tilesBufferPackage[localFrameIdx].Srv);
            GpuBuffer* tilesBufferPtr = renderContext->Lookup(tilesBufferPackage[localFrameIdx].Buffer);
            tilesBufferPtr->Unmap();
            renderContext->DestroyBuffer(tilesBufferPackage[localFrameIdx].Buffer);

            renderContext->DestroyGpuView(depthBinsBufferPackage[localFrameIdx].Srv);
            GpuBuffer* depthBinsBufferPtr = renderContext->Lookup(depthBinsBufferPackage[localFrameIdx].Buffer);
            depthBinsBufferPtr->Unmap();
            renderContext->DestroyBuffer(depthBinsBufferPackage[localFrameIdx].Buffer);
        }
    }

    void LightClusteringPass::SetParams(const LightClusteringPassParams& newParams)
    {
        IG_CHECK(newParams.CmdList != nullptr);
        IG_CHECK(newParams.NearPlane < newParams.FarPlane);
        IG_CHECK(newParams.TargetViewport.width > 0.f && newParams.TargetViewport.height > 0.f);
        params = newParams;
    }

    void LightClusteringPass::OnExecute(const LocalFrameIndex localFrameIdx)
    {
        ZoneScoped;
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(sceneProxy != nullptr);

        // #sy_todo Async로 처리해서 실제로 버퍼에 데이터 쓰기 전에 wait 하는 방식으로 구현하면 조금이나마 레이턴시 숨기기 가능하지 않을까?
        {
            ZoneScopedN("ResetData");
            ResetData();
        }

        const auto& lightProxyMapValues = sceneProxy->GetLightProxyMap(localFrameIdx).values();
        {
            ZoneScopedN("LightClustering");
            // Light Sorting 기준: 원점에서 카메라 방향(정규화)과 그 반대 방향으로 원점에서 반지름 만큼
            // 떨어져있는 점과 원점 3개 점을 가지고 판단.
            // sorting은 원점을 기준으로 판단. min bin = 가까운 점, max bin = 먼 점
            // Bin Idx 계산 기준은 그려둔 그림 보기
            lightProxyMapIdxList.clear();
            IG_CHECK(lightProxyMapIdxList.capacity() == kMaxNumLights);

            for (Index idx = 0; idx < std::min((Size)kMaxNumLights, lightProxyMapValues.size()); ++idx)
            {
                lightProxyMapIdxList.emplace_back((U16)idx);
            }

            std::sort(lightProxyMapIdxList.begin(), lightProxyMapIdxList.end(),
                      [this, &lightProxyMapValues](const Index lhsIdx, const Index rhsIdx)
                      {
                          const SceneProxy::LightProxy& lhsProxy = lightProxyMapValues[lhsIdx].second;
                          const SceneProxy::LightProxy& rhsProxy = lightProxyMapValues[rhsIdx].second;

                          const float lhsDist = Vector3::DistanceSquared(lhsProxy.GpuData.WorldPosition, params.CamWorldPos);
                          const float rhsDist = Vector3::DistanceSquared(rhsProxy.GpuData.WorldPosition, params.CamWorldPos);

                          return lhsDist < rhsDist;
                      });

            for (U16& lightIdxListIdx : lightProxyMapIdxList)
            {
                const SceneProxy::LightProxy& lightProxy = lightProxyMapValues[lightIdxListIdx].second;
                const float radius = lightProxy.GpuData.Property.Radius;

                // GPU에 기록 될 light idx는 Storage Space의 Index
                mappedLightIdxListStagingBuffer[localFrameIdx][lightIdxListIdx] = (U32)lightProxy.StorageSpace.OffsetIndex;

                Vector3 lightToCamera = params.CamWorldPos - lightProxy.GpuData.WorldPosition;
                lightToCamera.Normalize();

                Vector4 centerPoint = Vector4{lightProxy.GpuData.WorldPosition};
                centerPoint.w = 1.f;

                const Vector4 centerPointView = Vector4::Transform(centerPoint, params.ViewMat);
                Vector4 centerToCamView = -centerPointView;
                centerToCamView.w = 0.f;
                centerToCamView.Normalize();

                const Vector4 nearPointView = centerPointView + (centerToCamView * radius);
                const Vector4 farPointView = centerPointView + (-centerToCamView * radius);

                // 1. 어느 Depth Bin에 속하는지 판단 후 값 설정
                constexpr auto kDepthBinMaxIdx = (double)(kNumDepthBins - 1); // bin idx in [0, 32)
                const double invNearFarDist = 1.0 / (double)(params.FarPlane - params.NearPlane);
                I32 minDepthBinIdx = (I32)(kDepthBinMaxIdx * ((nearPointView.z - params.NearPlane) * invNearFarDist));
                I32 centerDepthBinIdx = (I32)(kDepthBinMaxIdx * ((centerPointView.z - params.NearPlane) * invNearFarDist));
                I32 maxDepthBinIdx = (I32)(kDepthBinMaxIdx * ((farPointView.z - params.NearPlane) * invNearFarDist));

                const bool bLightCenterIsNotInsideFrustum = centerDepthBinIdx < 0 || centerDepthBinIdx >= kNumDepthBins;
                const bool bLightNearFarIsNotInsideFrustum = (minDepthBinIdx < 0 && maxDepthBinIdx < 0) ||
                                                             (minDepthBinIdx >= kNumDepthBins && maxDepthBinIdx >= kNumDepthBins);
                const bool bShouldCulled = bLightCenterIsNotInsideFrustum && bLightNearFarIsNotInsideFrustum;
                if (bShouldCulled)
                {
                    continue;
                }

                minDepthBinIdx = std::max(0i32, minDepthBinIdx);
                maxDepthBinIdx = std::min((I32)kDepthBinMaxIdx, maxDepthBinIdx);
                IG_CHECK(minDepthBinIdx <= maxDepthBinIdx);

                // 여기 까지 온 Light는 [MinIdx, MaxIdx] 까지의 bin에 포함되어야 함.
                for (I32 depthBinIdx = minDepthBinIdx; depthBinIdx <= maxDepthBinIdx; ++depthBinIdx)
                {
                    DepthBin& depthBin = depthBins[depthBinIdx];
                    depthBin.FirstLightIdx = depthBin.FirstLightIdx == 0 ? lightIdxListIdx : std::min(depthBin.FirstLightIdx, lightIdxListIdx);
                    depthBin.LastLightIdx = std::max(depthBin.LastLightIdx, lightIdxListIdx);
                }

                // View Space에서 AABB를 만들고, Screen Space에 투영한 다음 투영 된 AABB가 Tile의 어느 지점에 걸리는지 확인해야함
                const Array<Vector4, 8> aabbCornerOffsets{
                    Vector4{-radius, -radius, -radius, 0.f},
                    Vector4{-radius, radius, -radius, 0.f},
                    Vector4{radius, radius, -radius, 0.f},
                    Vector4{radius, -radius, -radius, 0.f},
                    Vector4{-radius, radius, radius, 0.f},
                    Vector4{radius, radius, radius, 0.f},
                    Vector4{radius, -radius, radius, 0.f},
                    Vector4{-radius, -radius, radius, 0.f}};

                // 타일 정보만 구하면 되기 때문에 aabbScreen = (min.x, min.y, max.x, max.y)
                Vector4 aabbScreen{FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
                for (const Vector4& aabbCornerOffset : aabbCornerOffsets)
                {
                    const Vector4 aabbCornerView = centerPointView + aabbCornerOffset;
                    Vector4 aabbCornerScreen = Vector4::Transform(aabbCornerView, params.ProjMat);
                    /* Perpsective Division */
                    aabbCornerScreen /= aabbCornerScreen.w; // NDC

                    // Viewport Transformation NDC x,y in [-1, 1] -> SS x,y in [0, Viewport.Width), [0, Viewport.Height)
                    aabbCornerScreen.x = (aabbCornerScreen.x + 1.f) * 0.5f * (params.TargetViewport.width - 1.f);
                    aabbCornerScreen.y = (-aabbCornerScreen.y + 1.f) * 0.5f * (params.TargetViewport.height - 1.f);

                    aabbScreen.x = std::min(aabbCornerScreen.x, aabbScreen.x);
                    aabbScreen.y = std::min(aabbCornerScreen.y, aabbScreen.y);
                    aabbScreen.z = std::max(aabbCornerScreen.x, aabbScreen.z);
                    aabbScreen.w = std::max(aabbCornerScreen.y, aabbScreen.w);
                }

                aabbScreen.x = std::clamp(aabbScreen.x, 0.f, params.TargetViewport.width - 1.f);
                aabbScreen.y = std::clamp(aabbScreen.y, 0.f, params.TargetViewport.height - 1.f);
                aabbScreen.z = std::clamp(aabbScreen.z, 0.f, params.TargetViewport.width - 1.f);
                aabbScreen.w = std::clamp(aabbScreen.w, 0.f, params.TargetViewport.height - 1.f);
                IG_CHECK(aabbScreen.x <= aabbScreen.z);
                IG_CHECK(aabbScreen.y <= aabbScreen.w);

                /* 광원의 AABB가 Screen Space에서 픽셀 보다도 더 작세 나타나고, 그 너비가 0에 가깝다면 화면에 보이지 않는다고 가정 할 수 있음. */
                constexpr float kEpsilon = 0.000001f;
                if ((aabbScreen.z - aabbScreen.x) < kEpsilon || (aabbScreen.w - aabbScreen.y) < kEpsilon)
                {
                    continue;
                }

                aabbScreen *= kInvTileSize;
                const auto firstTileX = (Index)aabbScreen.x;
                const auto firstTileY = (Index)aabbScreen.y;
                const auto lastTileX = (Index)aabbScreen.z;
                const auto lastTileY = (Index)aabbScreen.w;

                IG_CHECK(firstTileX < numTileX && lastTileX < numTileX);
                IG_CHECK(firstTileY < numTileY && lastTileY < numTileY);
                for (Index tileX = firstTileX; tileX <= lastTileX; ++tileX)
                {
                    for (Index tileY = firstTileY; tileY <= lastTileY; ++tileY)
                    {
                        const Size tileIdx = numTileX * tileY + tileX;
                        const Size tileDwordOffset = tileIdx * kNumDWordsPerTile;
                        const Size lightDwordOffset = tileDwordOffset + (lightIdxListIdx / sizeof(U32));
                        const Size lightBitOffset = lightIdxListIdx % sizeof(U32);
                        tileDwords[lightDwordOffset] |= (1Ui32 << lightBitOffset);
                    }
                }
            }
        }
        
        {
            ZoneScopedN("CopyToSysmem");
            std::memcpy(tilesBufferPackage[localFrameIdx].MappedBuffer, tileDwords.data(), sizeof(U32) * numTileDwords);
            std::memcpy(depthBinsBufferPackage[localFrameIdx].MappedBuffer, depthBins.data(), sizeof(depthBins));

            GpuBuffer* lightIdxListStagingBufferPtr = renderContext->Lookup(lightIdxListStagingBuffer[localFrameIdx]);
            IG_CHECK(lightIdxListStagingBufferPtr != nullptr);
            GpuBuffer* lightIdxListBufferPtr = renderContext->Lookup(lightIdxListBufferPackage[localFrameIdx].Buffer);
            IG_CHECK(lightIdxListBufferPtr != nullptr);

            CommandList& cmdList = *params.CmdList;
            cmdList.Open();
            cmdList.CopyBuffer(*lightIdxListStagingBufferPtr, 0, sizeof(U32) * lightProxyMapValues.size(), *lightIdxListBufferPtr, 0);
            cmdList.Close();
        }

    }

    void LightClusteringPass::ResetData()
    {
        tileDwords.resize(numTileDwords);
        ZeroMemory(tileDwords.data(), sizeof(U32) * tileDwords.size());
        ZeroMemory(depthBins.data(), sizeof(depthBins));
    }
} // namespace ig
