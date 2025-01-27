#pragma once
#include "Igniter/Render/RenderPass.h"

namespace ig
{
    class CommandList;
    class GpuBuffer;
    class GpuView;
    class RootSignature;
    class DescriptorHeap;
    class ShaderBlob;
    class PipelineState;
    class RenderContext;
    class SceneProxy;

    struct LightClusteringPassParams
    {
        CommandList* CmdList = nullptr;
        Vector3 CamWorldPos{};
        Matrix ViewMat{};
        Matrix ProjMat{};
        // NearPlane < FarPlane => Reverse-Z를 사용하긴 하지만 Light Clustering엔 반대로 들어오지 않도록 Assertion으로 체크 할 것
        float NearPlane = 0.f;
        float FarPlane = 0.f;
        Viewport TargetViewport{};
    };

    class LightClusteringPass : public RenderPass
    {
      private:
        struct DepthBin
        {
            U16 FirstLightIdx = 0;
            U16 LastLightIdx = 0;
        };

        struct BufferPackage
        {
            RenderHandle<GpuBuffer> Buffer;
            RenderHandle<GpuView> Srv;
            U8* MappedBuffer = nullptr;
        };

      public:
        LightClusteringPass(RenderContext& renderContext, const SceneProxy& sceneProxy, const Viewport& mainViewport);
        LightClusteringPass(const LightClusteringPass&) = delete;
        LightClusteringPass(LightClusteringPass&&) noexcept = delete;
        ~LightClusteringPass() override;

        LightClusteringPass& operator=(const LightClusteringPass&) = delete;
        LightClusteringPass& operator=(LightClusteringPass&&) noexcept = delete;

        void SetParams(const LightClusteringPassParams& newParams);

        [[nodiscard]] RenderHandle<GpuView> GetTilesBufferSrv(const LocalFrameIndex localFrameIdx) const noexcept { return tilesBufferPackage[localFrameIdx].Srv; }
        [[nodiscard]] RenderHandle<GpuView> GetDepthBinsBufferSrv(const LocalFrameIndex localFrameIdx) const noexcept { return depthBinsBufferPackage[localFrameIdx].Srv; }

      protected:
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

      private:
        void ResetData();

      private:
        RenderContext* renderContext = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        static_assert((kMaxNumLights % 2) == 0);
        // Tile당 kMaxNumLights 만큼의 비트가 필요
        constexpr static Size kTileSize = 8;
        constexpr static Size kNumPixelsPerTile = kTileSize * kTileSize;
        constexpr static float kInvTileSize = 1.f / (float)kTileSize;
        constexpr static Size kNumDepthBins = 32;
        constexpr static Size kNumDWordsPerTile = kMaxNumLights / BytesToBits(sizeof(U32));
        constexpr static Size kDepthBinsBufferSize = kNumDepthBins * sizeof(DepthBin);

        Vector<U16> lightProxyMapIdxList;
        InFlightFramesResource<RenderHandle<GpuBuffer>> lightIdxListStagingBuffer;
        InFlightFramesResource<U32*> mappedLightIdxListStagingBuffer;
        InFlightFramesResource<BufferPackage> lightIdxListBufferPackage;

        // Screen Width/kTileSize * Height/kTileSize * kNumDwordsPerTile
        Size numTileX = 0;
        Size numTileY = 0;
        Size tileDwordsStride = 0;
        Size numTileDwords = 0;
        Vector<U32> tileDwords;
        //Array<eastl::bitset<kMaxNumLights>, kNumTiles> tileBitsets // 잘못됨. 8x8 타일로 전체 화면을 나눈 만큼의 타일이 있어야 하는데..
        InFlightFramesResource<BufferPackage> tilesBufferPackage;

        Array<DepthBin, kNumDepthBins> depthBins;
        InFlightFramesResource<BufferPackage> depthBinsBufferPackage;

        LightClusteringPassParams params;
    };
} // namespace ig
