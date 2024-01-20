#include <D3D12/Swapchain.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <Core/Window.h>

namespace fe
{
	Swapchain::Swapchain(const Window& window, const Device& device, const uint32_t backBufferCount)
		: backBufferCount(std::clamp(backBufferCount, MinBackBufferCount, MaxBackBufferCount)), descriptorHeap(std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, backBufferCount, "BackBufferDescHeap"))
	{
		InitSwapchain(window, device);
		CreateRenderTargetViews(device);
	}

	Swapchain::~Swapchain()
	{
		renderTargetViews.clear();
		descriptorHeap.reset();
		backBuffers.clear();
	}

	void Swapchain::InitSwapchain(const Window& window, const Device& device)
	{
		wrl::ComPtr<IDXGIFactory5> factory;

		uint32_t factoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

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

		wrl::ComPtr<IDXGISwapChain1> swapchain1;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(&device.GetDirectQueue(), window.GetNative(), &desc, nullptr, nullptr, &swapchain1));

		// Disable Alt+Enter full-screen toggle.
		ThrowIfFailed(factory->MakeWindowAssociation(window.GetNative(), DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapchain1.As(&swapchain));
	}

	void Swapchain::CheckTearingSupport(wrl::ComPtr<IDXGIFactory5> factory)
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

	void Swapchain::CreateRenderTargetViews(const Device& device)
	{
		renderTargetViews.reserve(backBufferCount);
		backBuffers.resize(backBufferCount);
		for (uint32_t idx = 0; idx < backBufferCount; ++idx)
		{
			// #todo Should i Wrapping backBuffer over GPUTexture? // or should i treat swapchain as one of abstracted another type of resource itself?
			ThrowIfFailed(swapchain->GetBuffer(idx, IID_PPV_ARGS(&backBuffers[idx])));

			Private::SetD3DObjectName(backBuffers[idx].Get(), std::format("Backbuffer {}", idx));
			renderTargetViews.emplace_back(descriptorHeap->AllocateDescriptor());

			ID3D12Device10& nativeDevice = device.GetNative();
			nativeDevice.CreateRenderTargetView(backBuffers[idx].Get(), nullptr, renderTargetViews[idx].GetCPUDescriptorHandle());
		}
	}

	void Swapchain::Present()
	{
		swapchain->Present(0, 0);
	}
} // namespace fe
