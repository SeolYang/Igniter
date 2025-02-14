#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    class Material;

    class MaterialComponent
    {
    public:
        MaterialComponent();
        MaterialComponent(const Handle<Material> ownedInstance);
        MaterialComponent(const MaterialComponent& other);
        MaterialComponent(MaterialComponent&& other) noexcept;
        ~MaterialComponent();

        MaterialComponent& operator=(const MaterialComponent& rhs);
        MaterialComponent& operator=(MaterialComponent&& rhs) noexcept;

        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

    public:
        Handle<Material> Instance{};

    private:
        void Destroy();

    private:
        constexpr static std::string_view ContainerKey{"MaterialComponent"};
        constexpr static std::string_view MeshGuidKey{"MaterialGuid"};
    };

    IG_DECLARE_META(MaterialComponent);
} // namespace ig
