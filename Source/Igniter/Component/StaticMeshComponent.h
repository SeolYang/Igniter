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
        ManagedAsset<StaticMesh> Mesh{};

    private:
        constexpr static std::string_view ContainerKey{"StaticMeshComponent"};
        constexpr static std::string_view MeshGuidKey{"StaticMeshGuid"};

    };

    IG_DECLARE_TYPE_META(StaticMeshComponent);
}    // namespace ig
