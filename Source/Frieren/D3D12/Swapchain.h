#pragma once
#include <D3D12/Common.h>

namespace fe
{
	class Window;
}

namespace fe::dx
{
	class Device;
	class DescriptorHeap;
	class Descriptor;
	class GPUTexture;
	class Swapchain
	{
	public:
		Swapchain(const Window& window, Device& device, const uint32_t numInflightFrames);
		~Swapchain();

		bool IsTearingSupport() const { return bIsTearingSupport; }

		const GPUTexture& GetBackBuffer() const;
		const Descriptor& GetRenderTargetView() const;

		// #todo Resize Swapchain(back buffers)!

		void Present();

	private:
		void InitSwapchain(const Window& window, const Device& device);
		void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
		void InitRenderTargetViews(Device& device);

	private:
		ComPtr<IDXGISwapChain4> swapchain;
		bool					bIsTearingSupport = false;

		const uint32_t							 numInflightFrames;
		std::vector<std::unique_ptr<GPUTexture>> backBuffers;
		std::vector<Descriptor>					 renderTargetViews;
	};
} // namespace fe::dx