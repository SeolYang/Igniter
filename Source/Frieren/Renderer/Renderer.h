#pragma once
#include <Core/Log.h>
#include <Renderer/FrameResource.h>

namespace fe::dx
{
	class Device;
	class DescriptorHeap;
	class Swapchain;
} // namespace fe::dx

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal);

	constexpr uint8_t NumFramesInFlight = 2;

	class Window;
	class FrameResource;
	class Renderer
	{
	public:
		Renderer(const Window& window);
		~Renderer();

		void Render();

	private:
		std::unique_ptr<dx::Device>	   device;
		std::unique_ptr<dx::Swapchain> swapchain;

		std::vector<FrameResource> frameResources; // reserved #'NumFramesInFlight'
		size_t					   globalFrames;
		size_t					   localFrames;

	};
} // namespace fe