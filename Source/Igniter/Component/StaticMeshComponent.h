#pragma once
#include <Core/Handle.h>
#include <Core/Meta.h>

namespace ig
{
    class StaticMesh;
    struct StaticMeshComponent
    {
    public:
        ~StaticMeshComponent();

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

    public:
        Handle<StaticMesh> Mesh{};

    private:
        constexpr static std::string_view ContainerKey{"StaticMeshComponent"};
        constexpr static std::string_view MeshGuidKey{"StaticMeshGuid"};

    };

    // #sy_todo_priority new meta features!!
    IG_DECLARE_TYPE_META(StaticMeshComponent);
}    // namespace ig
