#pragma once
#include <Core/Handle.h>
#include <D3D12/CommandQueue.h>
#include <Render/Swapchain.h>
#include <Render/TempConstantBufferAllocator.h>

namespace ig
{
    class RenderDevice;
    class CommandContext;
#pragma region test
    // #sy_test
    class GpuBuffer;
    class GpuTexture;
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuView;
#pragma endregion
}    // namespace ig

namespace ig
{
    class Window;
    class DeferredDeallocator;
    class RenderContext;
    class Renderer final
    {
    public:
        Renderer(const FrameManager& frameManager, Window& window, RenderContext& renderContext);
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        ~Renderer();

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        Swapchain& GetSwapchain() { return swapchain; }
        const TempConstantBufferAllocator& GetTempConstantBufferAllocator() const { return tempConstantBufferAllocator; }

        void BeginFrame(const uint8_t localFrameIdx);
        void Render(const uint8_t localFrameIdx, Registry& registry);
        void EndFrame(const uint8_t localFrameIdx);

    private:
        const FrameManager& frameManager;
        RenderContext& renderContext;

        TempConstantBufferAllocator tempConstantBufferAllocator;

        Viewport mainViewport{};
        Swapchain swapchain;
        GpuSync mainGfxFrameSyncs[NumFramesInFlight];

#pragma region test
        // #sy_test
        std::unique_ptr<ShaderBlob> vs;
        std::unique_ptr<ShaderBlob> ps;
        std::unique_ptr<RootSignature> bindlessRootSignature;
        std::unique_ptr<PipelineState> pso;
        eastl::array<RenderResource<GpuTexture>, NumFramesInFlight> depthStencils;
        eastl::array<RenderResource<GpuView>, NumFramesInFlight> dsvs;
#pragma endregion
    };
}    // namespace ig
