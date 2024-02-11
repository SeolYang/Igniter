#include <ImGui/ImGuiRenderer.h>
#include <ImGui/ImGuiCanvas.h>

namespace fe
{
	void ImGuiRenderer::Render(ImGuiCanvas& canvas)
	{
		canvas.Render();
	}
}