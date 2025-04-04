#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Window.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Swapchain.h"

IG_DECLARE_LOG_CATEGORY(SwapchainLog);

IG_DEFINE_LOG_CATEGORY(SwapchainLog);

namespace ig
{
    Swapchain::Swapchain(const Window& window, RenderContext& renderContext, const bool bEnableVSync /*= false*/)
        : renderContext(renderContext)
        , bVSyncEnabled(bEnableVSync)
    {
        CommandQueue& mainGfxQueue = renderContext.GetMainGfxQueue();

        ComPtr<IDXGIFactory5> factory;

        U32 factoryFlags = 0;
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
        desc.SampleDesc = {1, 0};
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = NumFramesInFlight;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        CheckTearingSupport(factory);
        desc.Flags = bTearingEnabled ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapchain1;
        IG_VERIFY_SUCCEEDED(factory->CreateSwapChainForHwnd(&mainGfxQueue.GetNative(), window.GetNative(), &desc, nullptr, nullptr, &swapchain1));

        // Disable Alt+Enter full-screen toggle.
        IG_VERIFY_SUCCEEDED(factory->MakeWindowAssociation(window.GetNative(), DXGI_MWA_NO_ALT_ENTER));
        IG_VERIFY_SUCCEEDED(swapchain1.As(&swapchain));

        for (LocalFrameIndex localFrameIdx = 0; localFrameIdx < NumFramesInFlight; ++localFrameIdx)
        {
            ComPtr<ID3D12Resource1> resource;
            IG_VERIFY_SUCCEEDED(swapchain->GetBuffer(localFrameIdx, IID_PPV_ARGS(&resource)));
            SetObjectName(resource.Get(), std::format("Backbuffer LF#{}", localFrameIdx));
            backBuffers[localFrameIdx] = renderContext.CreateTexture(GpuTexture{resource});
            rtv[localFrameIdx] =
                renderContext.CreateRenderTargetView(backBuffers[localFrameIdx],
                    D3D12_TEX2D_RTV{.MipSlice = 0, .PlaneSlice = 0});
        }
    }

    Swapchain::~Swapchain()
    {
        for (const LocalFrameIndex idx : LocalFramesView)
        {
            renderContext.DestroyGpuView(rtv[idx]);
            renderContext.DestroyTexture(backBuffers[idx]);
        }
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

    void Swapchain::Present()
    {
        const U32 syncInterval = bVSyncEnabled ? 1 : 0;
        const U32 presentFlags = bTearingEnabled && !bVSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
        const HRESULT result = swapchain->Present(syncInterval, presentFlags);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((result == DXGI_ERROR_DEVICE_REMOVED) ? renderContext.GetGpuDevice().GetDeviceRemovedReason() : result));
            IG_LOG(SwapchainLog, Fatal, "Present Failed: Device removed({})", buff);
        }
    }
} // namespace ig
