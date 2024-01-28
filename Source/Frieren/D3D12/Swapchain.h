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
		Swapchain(const Window& window, Device& device, const uint32_t backBufferCount = DefaultBackBufferCount);
		~Swapchain();

		bool IsTearingSupport() const { return bIsTearingSupport; }

		// #todo Resize Swapchain(back buffers)!

		void Present();

	private:
		void InitSwapchain(const Window& window, const Device& device);
		void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
		void InitRenderTargetViews(Device& device);

	public:
		static constexpr uint32_t MinBackBufferCount = 1;
		static constexpr uint32_t MaxBackBufferCount = 3;
		static constexpr uint32_t DefaultBackBufferCount = 2;

	private:
		ComPtr<IDXGISwapChain4> swapchain;
		bool					bIsTearingSupport = false;

		const uint32_t							 backBufferCount;
		std::vector<std::unique_ptr<GPUTexture>> backBuffers;
		std::vector<Descriptor>					 renderTargetViews;
	};
} // namespace fe::dx