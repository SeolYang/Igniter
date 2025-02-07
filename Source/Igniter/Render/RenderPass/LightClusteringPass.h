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
    struct TempConstantBuffer;

    struct LightClusteringPassParams
    {
        CommandList* InitCopyCmdList = nullptr;
        CommandList* ClearTileDwordsBufferCmdList = nullptr;
        CommandList* LightClusteringCmdList = nullptr;

        Vector3 CamWorldPos{};
        Matrix ViewMat{};
        Matrix ProjMat{};
        // NearPlane < FarPlane => Reverse-Z를 사용하긴 하지만 Light Clustering엔 반대로 들어오지 않도록 Assertion으로 체크 할 것
        float NearPlane = 0.f;
        float FarPlane = 0.f;
        Viewport TargetViewport{};

        const GpuView* PerFrameCbvPtr = nullptr;
        TempConstantBuffer* GpuParamsConstantBuffer = nullptr;
    };

    struct LightClusteringPassGpuParams
    {
        Matrix ToProj;
        float ViewportWidth;
        float ViewportHeight;

        U32 NumLights;

        U32 LightStorageBufferSrv;
        U32 LightIdxListBufferSrv;
        U32 TileDwordsBufferUav;
        U32 DepthBinsBufferUav;
    };

    /*
     * 주의할점:
     * 1. TileDwordsBuffer, DepthBinsBuffer에서 다뤄지는 idx들은
     * 모두 LightIdxList의 idx이다. 실제 LightStorage에 접근하기 위해선
     * LightStorage[LightIdxList[lightIdxListIdx]]로 접근 해야만 한다.
     *
     * 2. QHD 기준 타일의 크기가 8x8인 경우 타일 해상도는 320x180 이며, 총 타일의 수는 57,600개 이다.
     * MaxNumLights가 2^16(65,536) 이면, 타일 당 2^16비트(=8KB), 전체 타일을 합치면 450MB 크기의 버퍼가 필요하며
     * MaxNumLights가 2^13(8192) 일경우, 타일 당 2^13비트(=1KB), 전체 약 56MB의 크기를 가지는 버퍼가 필요하다
     * (실제로는 2개의 프레임을 동시에 렌더링 할 수 있다는 점을 고려하면 최소 2배의 메모리 공간을 필요로 한다)
     * 필요한 경우가 아니라면 MaxNumLights을 필요한 만큼(적당한 정도)만 설정해서 메모리 대역폭 및 캐시 지역성을 최대화 할 수 있도록 할 것.
     */
    class LightClusteringPass : public RenderPass
    {
      private:
        struct DepthBin
        {
            U32 FirstLightIdx = 0;
            U32 LastLightIdx = 0;
        };

        struct BufferPackage
        {
            Handle<GpuBuffer> Buffer;
            Handle<GpuView> Srv;
            Handle<GpuView> Uav;
        };

      public:
        LightClusteringPass(RenderContext& renderContext, const SceneProxy& sceneProxy, RootSignature& bindlessRootSignature, const Viewport& mainViewport);
        LightClusteringPass(const LightClusteringPass&) = delete;
        LightClusteringPass(LightClusteringPass&&) noexcept = delete;
        ~LightClusteringPass() override;

        LightClusteringPass& operator=(const LightClusteringPass&) = delete;
        LightClusteringPass& operator=(LightClusteringPass&&) noexcept = delete;

        void SetParams(const LightClusteringPassParams& newParams);

        [[nodiscard]] Handle<GpuView> GetLightIdxListSrv() const noexcept { return lightIdxListBufferPackage.Srv; }
        [[nodiscard]] Handle<GpuView> GetTilesBufferSrv() const noexcept { return tileDwordsBufferPackage.Srv; }
        [[nodiscard]] Handle<GpuBuffer> GetDepthBinInitBuffer() const noexcept { return depthBinInitBuffer; }
        [[nodiscard]] Handle<GpuView> GetDepthBinsBufferSrv() const noexcept { return depthBinsBufferPackage.Srv; }

      protected:
        void OnExecute(const LocalFrameIndex localFrameIdx) override;

      private:
        RenderContext* renderContext = nullptr;
        const SceneProxy* sceneProxy = nullptr;

        RootSignature* bindlessRootSignature = nullptr;

        static_assert((kMaxNumLights % 2) == 0);
        // Tile당 kMaxNumLights 만큼의 비트가 필요
        constexpr static Size kTileSize = 16;
        constexpr static Size kNumPixelsPerTile = kTileSize * kTileSize;
        constexpr static float kInvTileSize = 1.f / (float)kTileSize;
        constexpr static Size kNumDepthBins = 4096;
        constexpr static Size kNumDWordsPerTile = kMaxNumLights / BytesToBits(sizeof(U32));
        constexpr static Size kDepthBinsBufferSize = kNumDepthBins * sizeof(DepthBin);

        Ptr<ShaderBlob> clearU32BufferShader;
        Ptr<PipelineState> clearU32BufferPso;

        Ptr<ShaderBlob> lightClusteringShader;
        Ptr<PipelineState> lightClusteringPso;

        Vector<std::pair<U16, float>> lightProxyIdxViewZList;
        InFlightFramesResource<Handle<GpuBuffer>> lightIdxListStagingBuffer;
        InFlightFramesResource<U32*> mappedLightIdxListStagingBuffer;
        BufferPackage lightIdxListBufferPackage;

        // Screen Width/kTileSize * Height/kTileSize * kNumDwordsPerTile
        Size numTileX = 0;
        Size numTileY = 0;
        Size tileDwordsStride = 0;
        Size numTileDwords = 0;
        BufferPackage tileDwordsBufferPackage;
        BufferPackage depthBinsBufferPackage;
        Handle<GpuBuffer> depthBinInitBuffer;

        LightClusteringPassParams params;
    };
} // namespace ig
