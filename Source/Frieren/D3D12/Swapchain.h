#pragma once
#include <D3D12/Common.h>
#include <Core/Assert.h>

namespace fe
{
	class Window;
}

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class Descriptor;
	class GPUTexture;
	class Swapchain
	{
	public:
		Swapchain(const Window& window, Device& device, CommandQueue& directCmdQueue, const uint8_t desiredNumBackBuffers);
		~Swapchain();

		bool IsTearingSupport() const { return bIsTearingSupport; }

		GPUTexture& GetBackBuffer() { return *backBuffers[swapchain->GetCurrentBackBufferIndex()]; }
		const GPUTexture& GetBackBuffer() const { return *backBuffers[swapchain->GetCurrentBackBufferIndex()]; }
		const Descriptor& GetRenderTargetView() const
		{
			return renderTargetViews[swapchain->GetCurrentBackBufferIndex()];
		}

		// #todo Impl Resize Swapchain!
		// void Resize(const uint32_t width, const uint32_t height);
		// #todo Impl Change Color Space
		// void SetColorSpace(DXGI_COLOR_SPACE_TYPE newColorSpace);

		void Present();

	private:
		void InitSwapchain(const Window& window, CommandQueue& directCmdQueue);
		void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
		void InitRenderTargetViews(Device& device);

	private:
		ComPtr<IDXGISwapChain4> swapchain;
		bool					bIsTearingSupport = false;

		const uint8_t							 numBackBuffers;
		std::vector<std::unique_ptr<GPUTexture>> backBuffers;
		std::vector<Descriptor>					 renderTargetViews;
	};
} // namespace fe::dx