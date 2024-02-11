#pragma once
#include <ImGui/Common.h>

namespace fe::dx
{
	class Device;
}

namespace fe
{
	class Window;
	class ImGuiCanvas;
    class ImGuiRenderer
    {
	public:
		ImGuiRenderer() {}
		//ImGuiRenderer(dx::Device& device, Window& window) {}
		void Render(ImGuiCanvas& canvas);

    };
}