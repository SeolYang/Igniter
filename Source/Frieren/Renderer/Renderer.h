#pragma once
#include <Renderer/Common.h>

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class CommandContext;
	class CommandContextPool;
	class DescriptorHeap;
	class Swapchain;
	class Fence;

	// #test
	class GPUBuffer;
} // namespace fe::dx

namespace fe
{
	class Window;
	class World;
	class FrameResourceManager;
	class Renderer
	{
	public:
		Renderer(const FrameManager& engineFrameManager, Window& window, dx::Device& device);
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		~Renderer();

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void BeginFrame();
		void Render(FrameResourceManager& frameResourceManager);
		void Render(World& world);
		void EndFrame();

		dx::Swapchain&	  GetSwapchain() { return *swapchain; }
		dx::CommandQueue& GetDirectCommandQueue() { return *directCmdQueue; }

	private:
		const FrameManager&						frameManager;
		dx::Device&								renderDevice;
		std::unique_ptr<dx::CommandQueue>		directCmdQueue;
		std::unique_ptr<dx::CommandContextPool> directCmdCtxPool;
		std::unique_ptr<dx::Swapchain>			swapchain;
		std::vector<std::unique_ptr<dx::Fence>> frameFences;

		// #test
		std::unique_ptr<dx::GPUBuffer> quadVB;
		std::unique_ptr<dx::GPUBuffer> quadIB;
	};
} // namespace fe