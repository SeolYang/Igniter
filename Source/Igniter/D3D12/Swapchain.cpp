#include <Igniter.h>
#include <Core/Window.h>
#include <D3D12/Swapchain.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/GPUTexture.h>
#include <Render/GPUViewManager.h>

namespace ig
{
    Swapchain::Swapchain(const Window& window, GpuViewManager& gpuViewManager, CommandQueue& mainGfxQueue, const uint8_t desiredNumBackBuffers, const bool bEnableVSync)
        : numBackBuffers(desiredNumBackBuffers + 2),
          bVSyncEnabled(bEnableVSync)
    {
        InitSwapchain(window, mainGfxQueue);
        InitRenderTargetViews(gpuViewManager);
    }

    Swapchain::~Swapchain()
    {
        renderTargetViews.clear();
        backBuffers.clear();
    }

    GpuTexture& Swapchain::GetBackBuffer()
    {
        return backBuffers[swapchain->GetCurrentBackBufferIndex()];
    }

    const GpuTexture& Swapchain::GetBackBuffer() const
    {
        return backBuffers[swapchain->GetCurrentBackBufferIndex()];
    }

    void Swapchain::InitSwapchain(const Window& window, CommandQueue& mainGfxQueue)
    {
        ComPtr<IDXGIFactory5> factory;

        uint32_t factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        IG_VERIFY_SUCCEEDED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = 0;
        desc.Height = 0;

        /*
         * #sy_todo Support hdr
         * #sy_ref https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hdr-sample-win32
         */
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Stereo = false;
        desc.SampleDesc = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = numBackBuffers;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        CheckTearingSupport(factory);
        desc.Flags = bTearingEnabled ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapchain1;
        IG_VERIFY_SUCCEEDED(factory->CreateSwapChainForHwnd(&mainGfxQueue.GetNative(), window.GetNative(), &desc,
                                                         nullptr, nullptr, &swapchain1));

        // Disable Alt+Enter full-screen toggle.
        IG_VERIFY_SUCCEEDED(factory->MakeWindowAssociation(window.GetNative(), DXGI_MWA_NO_ALT_ENTER));
        IG_VERIFY_SUCCEEDED(swapchain1.As(&swapchain));
    }

    void Swapchain::CheckTearingSupport(ComPtr<IDXGIFactory5> factory)
    {
        BOOL allowTearing = FALSE;
        if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        {
            bTearingEnabled = false;
        }
        else
        {
            bTearingEnabled = allowTearing == TRUE;
        }
    }

    void Swapchain::InitRenderTargetViews(GpuViewManager& gpuViewManager)
    {
        renderTargetViews.reserve(numBackBuffers);
        backBuffers.reserve(numBackBuffers);
        for (uint32_t idx = 0; idx < numBackBuffers; ++idx)
        {
            ComPtr<ID3D12Resource1> resource;
            IG_VERIFY_SUCCEEDED(swapchain->GetBuffer(idx, IID_PPV_ARGS(&resource)));
            SetObjectName(resource.Get(), std::format("Backbuffer {}", idx));
            backBuffers.emplace_back(resource);
            renderTargetViews.emplace_back(gpuViewManager.RequestRenderTargetView(
                backBuffers[idx],
                D3D12_TEX2D_RTV{
                    .MipSlice = 0,
                    .PlaneSlice = 0 }));
        }
    }

    void Swapchain::Present()
    {
        const uint32_t syncInterval = bVSyncEnabled ? 1 : 0;
        const uint32_t presentFlags = bTearingEnabled && !bVSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
        IG_VERIFY_SUCCEEDED(swapchain->Present(syncInterval, presentFlags));
    }
} // namespace ig
