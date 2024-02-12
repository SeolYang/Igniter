#pragma once
#include <Core/Log.h>
#include <Renderer/FrameResource.h>

namespace fe::dx
{
	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class Swapchain;
} // namespace fe::dx

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning)
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal)

	constexpr uint8_t NumFramesInFlight = 2;

	class Window;
	class FrameResource;
	class World;
	class Renderer
	{
	public:
		Renderer(const Window& window);
		~Renderer();

		void BeginFrame();
		void Render();
		void Render(World& world);
		void EndFrame();

		dx::Device&	   GetDevice() { return *device; }
		dx::Swapchain& GetSwapchain() { return *swapchain; }
		dx::CommandQueue& GetDirectCommandQueue() { return *directCmdQueue; }
		size_t		   GetGlobalFrameIndex() const { return globalFrameIdx; }
		size_t		   GetLocalFrameIndex() const { return localFrameIdx; }

	private:
		std::unique_ptr<dx::Device>		  device;
		std::unique_ptr<dx::CommandQueue> directCmdQueue;

		std::unique_ptr<dx::Swapchain> swapchain;

		std::vector<FrameResource> frameResources; // reserved #'NumFramesInFlight'
		size_t					   globalFrameIdx;
		size_t					   localFrameIdx;

		// #todo command context pool?
	};
} // namespace fe