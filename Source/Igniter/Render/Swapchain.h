#pragma once
#include <D3D12/Common.h>
#include <Core/Handle.h>

namespace ig
{
    class Window;
}    // namespace ig

namespace ig
{
    class RenderContext;
    class CommandQueue;
    class DescriptorHeap;
    class GpuView;
    class GpuTexture;
    class Swapchain final
    {
    public:
        Swapchain(const Window& window, RenderContext& renderContext, const uint8_t desiredNumBackBuffers, const bool bEnableVSync = true);
        ~Swapchain();

        bool IsTearingSupport() const { return bTearingEnabled; }
        Handle<GpuTexture> GetBackBuffer() const { return backBuffers[swapchain->GetCurrentBackBufferIndex()]; }
        Handle<GpuView> GetRenderTargetView() const { return renderTargetViews[swapchain->GetCurrentBackBufferIndex()]; }

        // #sy_todo Impl Resize Swapchain!
        // void Resize(const uint32_t width, const uint32_t height);
        // #sy_todo Impl Change Color Space
        // void SetColorSpace(DXGI_COLOR_SPACE_TYPE newColorSpace);

        void Present();

    private:
        void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);

    private:
        RenderContext& renderContext;

        ComPtr<IDXGISwapChain4> swapchain;
        const uint8_t numBackBuffers;
        const bool bVSyncEnabled;
        bool bTearingEnabled = false;
        std::vector<Handle<GpuTexture>> backBuffers;
        std::vector<Handle<GpuView>> renderTargetViews;
    };
}    // namespace ig
