#pragma once
#include <Core/Log.h>

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(RendererInfo, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(RendererWarn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(RendererFatal, ELogVerbosiy::Fatal);

	constexpr uint8_t MaxFramesInFlight = 2;

	class Device;
	class DescriptorHeap;
    class Renderer
    {
	public:
		Renderer();
		~Renderer();

	private:
		std::unique_ptr<Device> device;
    };
}