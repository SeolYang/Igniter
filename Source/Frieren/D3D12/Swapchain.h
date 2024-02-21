#pragma once
#include <D3D12/Common.h>
#include <D3D12/GPUView.h>
#include <Core/Assert.h>
#include <Core/Handle.h>

namespace fe
{
	class Window;
} // namespace fe

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class GPUViewManager;
	class GPUTexture;
	class Swapchain
	{
	public:
		Swapchain(const Window& window, GPUViewManager& gpuViewManager, CommandQueue& directCmdQueue, const uint8_t desiredNumBackBuffers);
		~Swapchain();

		bool			  IsTearingSupport() const { return bIsTearingSupport; }
		GPUTexture&		  GetBackBuffer();
		const GPUTexture& GetBackBuffer() const;
		const GPUView&	  GetRenderTargetView() const { return *renderTargetViews[swapchain->GetCurrentBackBufferIndex()]; }

		// #todo Impl Resize Swapchain!
		// void Resize(const uint32_t width, const uint32_t height);
		// #todo Impl Change Color Space
		// void SetColorSpace(DXGI_COLOR_SPACE_TYPE newColorSpace);

		void Present();

	private:
		void InitSwapchain(const Window& window, CommandQueue& directCmdQueue);
		void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
		void InitRenderTargetViews(GPUViewManager& gpuViewManager);

	private:
		ComPtr<IDXGISwapChain4> swapchain;

		const uint8_t											   numBackBuffers;
		std::vector<GPUTexture>									   backBuffers;
		std::vector<UniqueHandle<GPUView, GPUViewHandleDestroyer>> renderTargetViews;

		bool bIsTearingSupport = false;
	};
} // namespace fe::dx