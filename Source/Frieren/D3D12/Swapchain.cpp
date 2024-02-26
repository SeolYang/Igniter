#include <D3D12/Swapchain.h>
#include <D3D12/Device.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUViewManager.h>
#include <D3D12/GPUView.h>
#include <Core/Window.h>

namespace fe::dx
{
	Swapchain::Swapchain(const Window& window, GpuViewManager& gpuViewManager, CommandQueue& directCmdQueue, const uint8_t desiredNumBackBuffers, const bool bEnableVSync)
		: numBackBuffers(desiredNumBackBuffers),
		  bVSyncEnabled(bEnableVSync)
	{
		InitSwapchain(window, directCmdQueue);
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

	void Swapchain::InitSwapchain(const Window& window, CommandQueue& directCmdQueue)
	{
		ComPtr<IDXGIFactory5> factory;

		uint32_t factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		verify_succeeded(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = 0;
		desc.Height = 0;

		/** #todo Support HDR monitor ref:
		 * https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hdr-sample-win32/ */
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
		verify_succeeded(factory->CreateSwapChainForHwnd(&directCmdQueue.GetNative(), window.GetNative(), &desc,
														 nullptr, nullptr, &swapchain1));

		// Disable Alt+Enter full-screen toggle.
		verify_succeeded(factory->MakeWindowAssociation(window.GetNative(), DXGI_MWA_NO_ALT_ENTER));
		verify_succeeded(swapchain1.As(&swapchain));
	}

	void Swapchain::CheckTearingSupport(ComPtr<IDXGIFactory5> factory)
	{
		BOOL allowTearing = FALSE;
		if (FAILED(
				factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
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
			verify_succeeded(swapchain->GetBuffer(idx, IID_PPV_ARGS(&resource)));
			SetObjectName(resource.Get(), std::format("Backbuffer {}", idx));
			backBuffers.emplace_back(resource);
			renderTargetViews.emplace_back(gpuViewManager.RequestRenderTargetView(backBuffers[idx], {}));
		}
	}

	void Swapchain::Present()
	{
		const uint32_t syncInterval = bVSyncEnabled ? 1 : 0;
		const uint32_t presentFlags = bTearingEnabled && !bVSyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
		verify_succeeded(swapchain->Present(syncInterval, presentFlags));
	}
} // namespace fe::dx
