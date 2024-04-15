#pragma once
#include <D3D12/CommandQueue.h>
#include <D3D12/Swapchain.h>
#include <Core/Handle.h>
#include <Render/TempConstantBufferAllocator.h>

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
    class RenderContext;
#pragma endregion
} // namespace ig

namespace ig
{
    class Window;
    class DeferredDeallocator;

    class Renderer final
    {
    public:
        Renderer(const FrameManager& frameManager, Window& window, RenderDevice& device, HandleManager& handleManager,
                 RenderContext&      renderContext);
        Renderer(const Renderer&)     = delete;
        Renderer(Renderer&&) noexcept = delete;
        ~Renderer();

        Renderer& operator=(const Renderer&)     = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        Swapchain&                   GetSwapchain() { return swapchain; }
        TempConstantBufferAllocator& GetTempConstantBufferAllocator() { return tempConstantBufferAllocator; }

        void BeginFrame();
        void Render(Registry& registry);
        void EndFrame();

    private:
        const FrameManager& frameManager;
        RenderDevice&       renderDevice;
        HandleManager&      handleManager;
        RenderContext&      renderContext;

        TempConstantBufferAllocator tempConstantBufferAllocator;

        Viewport mainViewport{};

        Swapchain swapchain;
        GpuSync   mainGfxFrameSyncs[NumFramesInFlight];

#pragma region test
        // #sy_test
        std::unique_ptr<ShaderBlob>      vs;
        std::unique_ptr<ShaderBlob>      ps;
        std::unique_ptr<RootSignature>   bindlessRootSignature;
        std::unique_ptr<PipelineState>   pso;
        std::unique_ptr<GpuTexture>      depthStencilBuffer;
        Handle<GpuView, GpuViewManager*> dsv;
#pragma endregion
    };
} // namespace ig
