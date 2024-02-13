#pragma once
#include <ImGui/Common.h>
#include <Core/Container.h>
#include <memory>

namespace fe::dx
{
	class Device;
	class DescriptorHeap;
	class CommandContext;
} // namespace fe::dx

namespace fe
{
	class FrameManager;
	class Window;
	class Renderer;
	class ImGuiCanvas;
	class ImGuiRenderer
	{
	public:
		ImGuiRenderer(const FrameManager& engineFrameManager, dx::Device& device, Window& window);
		~ImGuiRenderer();

		void Render(ImGuiCanvas& canvas, Renderer& renderer);

	private:
		const FrameManager&								 frameManager;
		std::unique_ptr<dx::DescriptorHeap>				 descriptorHeap;
		std::vector<std::unique_ptr<dx::CommandContext>> commandContexts;
	};
} // namespace fe