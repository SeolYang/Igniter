#pragma once
namespace ig
{
    class ImGuiCanvas
    {
    public:
        ImGuiCanvas() = default;
        ImGuiCanvas(const ImGuiCanvas&) = delete;
        ImGuiCanvas(ImGuiCanvas&&) noexcept = delete;
        virtual ~ImGuiCanvas() = default;

        ImGuiCanvas& operator=(const ImGuiCanvas&) = delete;
        ImGuiCanvas& operator=(ImGuiCanvas&&) noexcept = delete;

        virtual void OnImGui() final;

    protected:
        virtual void OnGui() = 0;
    };
} // namespace ig
