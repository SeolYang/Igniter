#include <Igniter.h>
#include <ImGui/ImGuiCanvas.h>

namespace ig
{
    ImGuiCanvas::~ImGuiCanvas()
    {
    }

    void ImGuiCanvas::Render()
    {
        if (bDirty)
        {
            std::sort(layers.begin(), layers.end(),
                      [](const auto& lhs, const auto& rhs)
                      {
                          return lhs->GetPriority() < rhs->GetPriority();
                      });

            bDirty = false;
        }

        for (auto& layer : layers)
        {
            if (layer->IsVisible())
            {
                layer->Render();
            }
        }
    }
} // namespace ig
