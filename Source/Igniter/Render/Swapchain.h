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

        Handle<GpuTexture> GetBackBuffer() const { return backBuffers[(LocalFrameIndex)swapchain->GetCurrentBackBufferIndex()]; }
        Handle<GpuView> GetBackBufferRtv() const { return rtv[(LocalFrameIndex)swapchain->GetCurrentBackBufferIndex()]; }
        InFlightFramesResource<Handle<GpuTexture>> GetBackBuffers() const { return backBuffers; }
        InFlightFramesResource<Handle<GpuView>> GetBackBufferRenderTargetViews() const { return rtv; }

        // #sy_todo Impl Resize Swapchain!
        // void Resize(const U32 width, const U32 height);
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
        InFlightFramesResource<Handle<GpuTexture>> backBuffers;
        InFlightFramesResource<Handle<GpuView>> rtv;
    };
} // namespace ig
