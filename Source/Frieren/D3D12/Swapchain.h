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
	class Swapchain
	{
	public:
		Swapchain(const Window& window, const Device& device, const uint32_t backBufferCount = MinBackBufferCount);
		~Swapchain();

		bool IsTearingSupport() const { return bIsTearingSupport; }

		// #todo Resize Swapchain(back buffers)!

		void Present();

	private:
		void InitSwapchain(const Window& window, const Device& device);
		void CheckTearingSupport(ComPtr<IDXGIFactory5> factory);
		void CreateRenderTargetViews(const Device& device);

	public:
		static constexpr uint32_t MinBackBufferCount = 2;
		static constexpr uint32_t MaxBackBufferCount = 4;

	private:
		ComPtr<IDXGISwapChain4> swapchain;
		bool					bIsTearingSupport = false;

		const uint32_t						backBufferCount;
		std::vector<ComPtr<ID3D12Resource>> backBuffers; // #todo ID3D12Resource -> GPUTexture?

		std::unique_ptr<DescriptorHeap> descriptorHeap;
		std::vector<Descriptor>			renderTargetViews;
	};
} // namespace fe::dx