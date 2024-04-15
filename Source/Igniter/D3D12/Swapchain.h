#pragma once
#include <D3D12/Common.h>
#include <Core/Handle.h>

namespace ig
{
    class Window;
} // namespace ig

namespace ig
{
    class RenderDevice;
    class CommandQueue;
    class DescriptorHeap;
    class GpuViewManager;
    class GpuView;
    class GpuTexture;

    class Swapchain final
    {
    public:
        Swapchain(const Window& window, GpuViewManager&           gpuViewManager, CommandQueue& directCmdQueue,
                  const uint8_t desiredNumBackBuffers, const bool bEnableVSync = true);
        ~Swapchain();

        bool              IsTearingSupport() const { return bTearingEnabled; }
        GpuTexture&       GetBackBuffer();
        const GpuTexture& GetBackBuffer() const;

        const GpuView& GetRenderTargetView() const
        {
            return *renderTargetViews[swapchain->GetCurrentBackBufferIndex()];
        }

        // #sy_todo Impl Resize Swapchain!
        // void Resize(const uint32_t width, const uint32_t height);
        // #sy_todo Impl Change Color Space
        // void SetColorSpace(DXGI_COLOR_SPACE_TYPE newColorSpace);
        void Present();

    private:
        void InitSwapchain(const Window& window, CommandQueue& directCmdQueue);
        void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
        void InitRenderTargetViews(GpuViewManager& gpuViewManager);

    private:
        ComPtr<IDXGISwapChain4> swapchain;

        const uint8_t numBackBuffers;
        const bool    bVSyncEnabled;
        bool          bTearingEnabled = false;

        std::vector<GpuTexture>                       backBuffers;
        std::vector<Handle<GpuView, GpuViewManager*>> renderTargetViews;
    };
} // namespace ig
