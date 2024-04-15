#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename Component>
    void OnImGui(Registry&, const Entity)
    {
    }

    template <typename Component>
    void SerializeComponent(Registry&, const Entity, json&)
    {
    }

    template <typename Component>
    void DeserializeComponent(Registry&, const Entity, const json&)
    {
    }

    class ComponentRegistry final
    {
    public:
        using ComponentEvent      = std::function<void(Registry&, const Entity)>;
        using SerializeCallback   = std::function<void(Registry&, const Entity, json&)>;
        using DeserializeCallback = std::function<void(Registry&, const Entity, const json&)>;

        struct ComponentInfo final
        {
            std::string_view    Name;
            ComponentEvent      OnImGui{};
            SerializeCallback   Serialize{};
            DeserializeCallback Deserialize{};
        };

        template <typename Component>
        static void Register(const std::string_view nameOfComponent)
        {
            Register(entt::type_hash<Component>::value(), ComponentInfo{
                         .Name = nameOfComponent,
                         .OnImGui = OnImGui<Component>,
                         .Serialize = SerializeComponent<Component>,
                         .Deserialize = DeserializeComponent<Component>
                     });
        }

        static std::span<const std::pair<entt::id_type, ComponentInfo>> GetComponentInfos();

    private:
        static void Register(const entt::id_type componentTypeID, const ComponentInfo componentInfo);
        static std::vector<std::pair<entt::id_type, ComponentRegistry::ComponentInfo>>& GetComponentInfosInternal();
    };
} // namespace ig

#define IG_DECLARE_COMPONENT(COMPONENT_TYPE)                                                                     \
    namespace details::component                                                                                 \
    {                                                                                                            \
        struct COMPONENT_TYPE##REGISTRATION                                                                      \
        {                                                                                                        \
            COMPONENT_TYPE##REGISTRATION() { ig::ComponentRegistry::Register<COMPONENT_TYPE>(#COMPONENT_TYPE); } \
                                                                                                                 \
        private:                                                                                                 \
            static COMPONENT_TYPE##REGISTRATION _##COMPONENT_TYPE##REGISTRATION;                                 \
        };                                                                                                       \
    }

#define IG_DEFINE_COMPONENT(COMPONENT_TYPE) \
    details::component::COMPONENT_TYPE##REGISTRATION details::component::COMPONENT_TYPE##REGISTRATION::_##COMPONENT_TYPE##REGISTRATION
