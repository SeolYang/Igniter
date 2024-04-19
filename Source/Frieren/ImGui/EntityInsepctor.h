#pragma once
#include <Frieren.h>
#include <ImGui/ImGuiLayer.h>

namespace fe
{
    class EntityList;
    class EntityInspector final : public ImGuiLayer
    {
    public:
        explicit EntityInspector(const EntityList& entityList);
        EntityInspector(const EntityInspector&) = delete;
        EntityInspector(EntityInspector&&) noexcept = delete;
        ~EntityInspector() override = default;

        EntityInspector& operator=(const EntityInspector&) = delete;
        EntityInspector& operator=(EntityInspector&&) noexcept = delete;

        void Render() override;

    private:
        const EntityList* entityList;
    };
} // namespace ig
