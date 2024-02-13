#pragma once
#include <Renderer/Common.h>

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class Swapchain;
	class Fence;
} // namespace fe::dx

namespace fe
{
	class Window;
	class World;
	class Renderer
	{
	public:
		Renderer(const FrameManager& engineFrameManager, Window& window);
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		~Renderer();

		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void BeginFrame();
		void Render();
		void Render(World& world);
		void EndFrame();

		dx::Device&		  GetDevice() { return *device; }
		dx::Swapchain&	  GetSwapchain() { return *swapchain; }
		dx::CommandQueue& GetDirectCommandQueue() { return *directCmdQueue; }

	private:
		const FrameManager&						frameManager;
		std::unique_ptr<dx::Device>				device;
		std::unique_ptr<dx::CommandQueue>		directCmdQueue;
		std::unique_ptr<dx::Swapchain>			swapchain;
		std::vector<std::unique_ptr<dx::Fence>> frameFences;
	};
} // namespace fe