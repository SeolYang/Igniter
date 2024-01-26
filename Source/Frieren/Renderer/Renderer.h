#pragma once
#include <Core/Log.h>
#include <Renderer/FrameResource.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal);

	constexpr uint8_t MaxFramesInFlight = 2;

	class Device;
	class DescriptorHeap;
	class Swapchain;
	class Window;
	class FrameResource;
	class Renderer
	{
	public:
		Renderer(const Window& window);
		~Renderer();

		void Render();

	private:
		void BeginFrame(FrameResource& frameResource);
		void EndFrame(FrameResource& frameResource);

	private:
		std::unique_ptr<Device>	   device;
		std::unique_ptr<Swapchain> swapchain;
		std::vector<FrameResource> frameResources; // reserved #'MaxFramesInFlight'

	};
} // namespace fe