#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class EntityList;
    class EntityInspector : public ImGuiLayer
    {
    public:
        EntityInspector(const EntityList& entityList);
        ~EntityInspector() = default;

        void Render() override;

    private:
        const EntityList& entityList;
    };


} // namespace ig