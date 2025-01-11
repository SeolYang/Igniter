#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    class StaticMesh;
    struct StaticMeshComponent
    {
    public:
        StaticMeshComponent() = default;
        StaticMeshComponent(const StaticMeshComponent& other);
        StaticMeshComponent(StaticMeshComponent&& other) noexcept;

        ~StaticMeshComponent();

        // #sy_todo 만약, 기존에 Mesh 핸들을 가지고있었다면 해제해주어야함, copy operation에도 동일 처리 필요
        StaticMeshComponent& operator=(const StaticMeshComponent& rhs);
        StaticMeshComponent& operator=(StaticMeshComponent&& rhs) noexcept;

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

    public:
        ManagedAsset<StaticMesh> Mesh{};

    private:
        void Destroy();

    private:
        constexpr static std::string_view ContainerKey{"StaticMeshComponent"};
        constexpr static std::string_view MeshGuidKey{"StaticMeshGuid"};
    };

    IG_DECLARE_TYPE_META(StaticMeshComponent);
} // namespace ig
