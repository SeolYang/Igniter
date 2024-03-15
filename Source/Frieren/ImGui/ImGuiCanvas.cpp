#include <ImGui/ImGuiCanvas.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    ImGuiCanvas::~ImGuiCanvas() {}

    void ImGuiCanvas::Render()
    {
        for (auto& layer : layers)
        {
            if (layer->IsVisible())
            {
                layer->Render();
            }
        }
    }
} // namespace fe