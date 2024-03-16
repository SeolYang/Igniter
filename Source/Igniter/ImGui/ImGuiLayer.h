#pragma once
#include <Core/String.h>
#include <ImGui/Common.h>

namespace ig
{
    class ImGuiLayer
    {
    public:
        ImGuiLayer(const String layerName)
            : name(layerName) {}
        virtual ~ImGuiLayer() = default;
        virtual void Render() = 0;

        bool IsVisible() const { return bIsVisible; }
        void SetVisibility(const bool bIsVisibleLayer) { bIsVisible = bIsVisibleLayer; }

        String GetName() const { return name; }

    protected:
        bool bIsVisible = true;

    private:
        const String name;
    };
} // namespace ig