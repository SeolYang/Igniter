#include <D3D12/Swapchain.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUTexture.h>
#include <Core/Window.h>

namespace fe::dx
{
	Swapchain::Swapchain(const Window& window, Device& device, const uint32_t backBufferCount)
		: backBufferCount(std::clamp(backBufferCount, MinBackBufferCount, MaxBackBufferCount))
	{
		InitSwapchain(window, device);
		InitRenderTargetViews(device);
	}

	Swapchain::~Swapchain()
	{
		renderTargetViews.clear();
		backBuffers.clear();
	}

	void Swapchain::InitSwapchain(const Window& window, const Device& device)
	{
		ComPtr<IDXGIFactory5> factory;

		uint32_t factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		FE_SUCCEEDED_ASSERT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		/** #todo Customizable(or preset)Swapchain Resolution */
		desc.Width = 0;
		desc.Height = 0;

		/** #todo Support HDR monitor ref: https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hdr-sample-win32/ */
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Stereo = false;
		desc.SampleDesc = { 1, 0 };
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = backBufferCount;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		CheckTearingSupport(factory);
		desc.Flags = bIsTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swapchain1;
		FE_SUCCEEDED_ASSERT(factory->CreateSwapChainForHwnd(&device.GetDirectQueue(), window.GetNative(), &desc, nullptr, nullptr, &swapchain1));

		// Disable Alt+Enter full-screen toggle.
		FE_SUCCEEDED_ASSERT(factory->MakeWindowAssociation(window.GetNative(), DXGI_MWA_NO_ALT_ENTER));

		FE_SUCCEEDED_ASSERT(swapchain1.As(&swapchain));
	}

	void Swapchain::CheckTearingSupport(ComPtr<IDXGIFactory5> factory)
	{
		BOOL allowTearing = FALSE;
		if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			bIsTearingSupport = false;
		}
		else
		{
			bIsTearingSupport = allowTearing == TRUE;
		}
	}

	void Swapchain::InitRenderTargetViews(Device& device)
	{
		renderTargetViews.reserve(backBufferCount);
		backBuffers.reserve(backBufferCount);
		for (uint32_t idx = 0; idx < backBufferCount; ++idx)
		{
			// #todo Should i Wrapping backBuffer over GPUTexture? // or should i treat swapchain as one of abstracted another type of resource itself?
			ComPtr<ID3D12Resource1> resource;
			FE_SUCCEEDED_ASSERT(swapchain->GetBuffer(idx, IID_PPV_ARGS(&resource)));
			SetObjectName(resource.Get(), std::format("Backbuffer {}", idx));

			backBuffers.emplace_back(std::make_unique<GPUTexture>(resource));
			std::optional<Descriptor> descriptor = device.CreateRenderTargetView(*backBuffers[idx], {});
			if (!descriptor)
			{
				throw std::runtime_error("Failed to create render target view from back-buffer.");
			}
			renderTargetViews.emplace_back(std::move(*descriptor));
		}
	}

	void Swapchain::Present()
	{
		swapchain->Present(0, 0);
	}
} // namespace fe
