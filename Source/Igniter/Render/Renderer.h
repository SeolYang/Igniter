#pragma once
#include <Render/TempConstantBufferAllocator.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContextPool.h>
#include <D3D12/Swapchain.h>
#include <Core/Handle.h>

namespace ig
{
    class RenderDevice;
    class CommandContext;
    class Fence;

#pragma region test
    // #sy_test
    class GpuBuffer;
    class GpuTexture;
    class ShaderBlob;
    class RootSignature;
    class PipelineState;
    class GpuView;
    class GpuViewManager;
#pragma endregion
} // namespace ig

namespace ig
{
    class Window;
    class DeferredDeallocator;
    class Renderer
    {
    public:
        Renderer(const FrameManager& engineFrameManager, DeferredDeallocator& engineDefferedDeallocator, Window& window, RenderDevice& device, HandleManager& handleManager, GpuViewManager& gpuViewManager);
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        ~Renderer();

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        CommandQueue& GetMainGfxQueue() { return mainGfxQueue; }
        Swapchain& GetSwapchain() { return swapchain; }

        void BeginFrame();
        void Render(Registry& registry);
        void EndFrame();

        void FlushQueues();

        auto& GetTempConstantBufferAllocator() { return tempConstantBufferAllocator; }

    private:
        const FrameManager& frameManager;
        DeferredDeallocator& deferredDeallocator;
        RenderDevice& renderDevice;
        HandleManager& handleManager;
        GpuViewManager& gpuViewManager;

        Viewport mainViewport{};

        CommandQueue mainGfxQueue;
        CommandContextPool gfxCmdCtxPool;
        Swapchain swapchain;
        GpuSync mainGfxFrameSyncs[NumFramesInFlight];

        TempConstantBufferAllocator tempConstantBufferAllocator;

#pragma region test
        // #sy_test
        std::unique_ptr<ShaderBlob> vs;
        std::unique_ptr<ShaderBlob> ps;
        std::unique_ptr<RootSignature> bindlessRootSignature;
        std::unique_ptr<PipelineState> pso;
        std::unique_ptr<GpuTexture> depthStencilBuffer;
        Handle<GpuView, GpuViewManager*> dsv;
#pragma endregion
    };
} // namespace ig