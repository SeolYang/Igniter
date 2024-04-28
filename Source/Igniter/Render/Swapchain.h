#pragma once
#include <Core/Handle.h>
#include <D3D12/Common.h>
#include <Render/RenderCommon.h>

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
        RenderResource<GpuTexture> GetBackBuffer() const { return backBuffers[swapchain->GetCurrentBackBufferIndex()]; }
        RenderResource<GpuView> GetRenderTargetView() const { return renderTargetViews[swapchain->GetCurrentBackBufferIndex()]; }

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
        std::vector<RenderResource<GpuTexture>> backBuffers;
        std::vector<RenderResource<GpuView>> renderTargetViews;
    };
}    // namespace ig
