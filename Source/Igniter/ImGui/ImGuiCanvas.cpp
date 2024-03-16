#include <ImGui/ImGuiCanvas.h>
#include <ImGui/ImGuiLayer.h>

namespace ig
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
} // namespace ig