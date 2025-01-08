#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/Render/Common.h"

namespace ig
{
    class Window;
} // namespace ig

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
        Swapchain(const Window& window, RenderContext& renderContext, const bool bEnableVSync = false);
        ~Swapchain();

        bool IsTearingSupport() const { return bTearingEnabled; }
        RenderHandle<GpuTexture> GetBackBuffer() const { return backBuffers.Resources[swapchain->GetCurrentBackBufferIndex()]; }
        RenderHandle<GpuView> GetRenderTargetView() const { return renderTargetViews.Resources[swapchain->GetCurrentBackBufferIndex()]; }
        InFlightFramesResource<RenderHandle<GpuTexture>> GetBackBuffers() const { return backBuffers; }
        InFlightFramesResource<RenderHandle<GpuView>> GetBackBufferRenderTargetViews() const { return renderTargetViews; }

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
        const bool bVSyncEnabled;
        bool bTearingEnabled = false;
        InFlightFramesResource<RenderHandle<GpuTexture>> backBuffers;
        InFlightFramesResource<RenderHandle<GpuView>> renderTargetViews;
    };
} // namespace ig
