#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Hash.h"
#include "Igniter/Core/String.h"

namespace ig::meta
{
    using Type = entt::meta_type;
    using Property = entt::meta_prop;
    using Function = entt::meta_func;
    using HashedString = entt::hashed_string;

    constexpr inline entt::hashed_string NameProperty = "Name"_hs;
    constexpr inline entt::hashed_string TitleCaseNameProperty = "TitleCaseName"_hs;
    constexpr inline entt::hashed_string DescriptionProperty = "Description"_hs;
    constexpr inline entt::hashed_string ComponentProperty = "Component"_hs;
    constexpr inline entt::hashed_string AddComponentFunc = "AddComponent"_hs;
    constexpr inline entt::hashed_string RemoveComponentFunc = "RemoveComponent"_hs;
    // Signature:OnInspectorFunc(Registry*, Entity); // #sy_todo -> Ref<Registry>로 변경 할 것
    constexpr inline entt::hashed_string OnInspectorFunc = "OnInspectorFunc"_hs;
    constexpr inline entt::hashed_string ArchetypeProperty = "Archetype"_hs;
    constexpr inline entt::hashed_string ArchetypeCreateFunc = "ArchetypeCreate"_hs;
    constexpr inline entt::hashed_string StartupAppClass = "StartupClass"_hs;

    [[nodiscard]] inline bool HasProperty(const ig::meta::Type& type, const entt::hashed_string propertyIdentifier)
    {
        return static_cast<bool>(type.prop(propertyIdentifier));
    }

    [[nodiscard]] inline bool IsReflectedType(const ig::meta::Type& type)
    {
        const auto bOwningNameProperty = static_cast<bool>(type.prop(NameProperty));
        const auto bOwningTitleCaseNameProperty = static_cast<bool>(type.prop(TitleCaseNameProperty));
        return bOwningNameProperty && bOwningTitleCaseNameProperty;
    }

    template <typename T>
    [[nodiscard]] bool IsReflectedType()
    {
        using Ty = std::decay_t<T>;
        return IsReflectedType(entt::resolve<Ty>());
    }

    [[nodiscard]] inline bool IsComponent(const ig::meta::Type& type)
    {
        const auto bOwningComponentProperty = static_cast<bool>(type.prop(ComponentProperty));
        const auto bOwningAddComponentFunc = static_cast<bool>(type.func(AddComponentFunc));
        const auto bOwningRemoveComponentFunc = static_cast<bool>(type.func(RemoveComponentFunc));
        const bool bHasComponentIdentity = bOwningComponentProperty && bOwningAddComponentFunc && bOwningRemoveComponentFunc;
        return IsReflectedType(type) && bHasComponentIdentity;
    }

    template <typename T>
    [[nodiscard]] bool IsComponent()
    {
        using Ty = std::decay_t<T>;
        return IsComponent(entt::resolve<Ty>());
    }

    [[nodiscard]] inline bool IsArchetype(const ig::meta::Type& type)
    {
        const auto bOwningArchetypeProperty = static_cast<bool>(type.prop(ArchetypeProperty));
        const auto bOwningCreateFunc = static_cast<bool>(type.func(ArchetypeCreateFunc));
        const bool bHasArchetypeIdentity = bOwningArchetypeProperty && bOwningCreateFunc;
        return IsReflectedType(type) && bHasArchetypeIdentity;
    }

    template <typename T>
    [[nodiscard]] bool IsArchetype()
    {
        using Ty = std::decay_t<T>;
        return IsArchetype(entt::resolve<Ty>());
    }

    template <typename... Args>
    bool Invoke(const ig::meta::Type& type, const entt::hashed_string functionIdentifier, Args&&... args)
    {
        const Function function = type.func(functionIdentifier);
        if (!function)
        {
            return false;
        }

        function.invoke(type, std::forward<Args>(args)...);
        return true;
    }

    template <typename T, typename... Args>
    bool Invoke(const entt::hashed_string functionIdentifier, Args&&... args)
    {
        using Ty = std::decay_t<T>;
        const ig::meta::Type type = entt::resolve<Ty>();
        if (!type)
        {
            return false;
        }

        return Invoke(type, functionIdentifier, std::forward<Args>(args)...);
    }
} // namespace ig::meta
namespace ig
{
    template <typename T>
        requires std::is_default_constructible_v<T>
    bool AddComponent(Ref<Registry> registryRef, const Entity entity)
    {
        IG_CHECK(entity != entt::null);
        Registry& registry = registryRef.get();
        if (registry.all_of<T>(entity))
        {
            return false;
        }

        // #sy_note 새롭게 추가한 컴포넌트의 타입을 반환하고 싶지만, Tag Component의 경우 메모리 상에 존재하지 않기 때문에
        // 불가능 하다.
        registry.emplace<T>(entity);
        return registry.all_of<T>(entity);
    }

    template <typename T>
    bool RemoveComponent(Ref<Registry> registryRef, const Entity entity)
    {
        IG_CHECK(entity != entt::null);
        return registryRef.get().remove<T>(entity) > 0;
    }
} // namespace ig

#define IG_DECLARE_META(T)              \
    template <typename Ty>              \
    void DefineMeta();                  \
    namespace details::meta             \
    {                                   \
        struct T##_DefineMeta           \
        {                               \
            T##_DefineMeta();           \
                                        \
          private:                      \
            static T##_DefineMeta _reg; \
        };                              \
    }

#define IG_DEFINE_META(T)                                                                 \
    namespace details::meta                                                               \
    {                                                                                     \
        T##_DefineMeta::T##_DefineMeta()                                                  \
        {                                                                                 \
            entt::meta<T>().type(TypeHash<T>);                                            \
            entt::meta<T>().prop(ig::meta::NameProperty, #T);                             \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase()); \
            DefineMeta<T>();                                                              \
        }                                                                                 \
        T##_DefineMeta T##_DefineMeta::_reg;                                              \
    }

#define IG_DEFINE_COMPONENT_META(T)                                                                \
    namespace details::meta                                                                        \
    {                                                                                              \
        T##_DefineMeta::T##_DefineMeta()                                                           \
        {                                                                                          \
            entt::meta<T>().type(ig::TypeHash<T>);                                                 \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                                 \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase());          \
            entt::meta<T>().prop(ig::meta::ComponentProperty);                                     \
            entt::meta<T>().template func<&ig::AddComponent<T>>(ig::meta::AddComponentFunc);       \
            entt::meta<T>().template func<&ig::RemoveComponent<T>>(ig::meta::RemoveComponentFunc); \
            DefineMeta<T>();                                                                       \
        }                                                                                          \
        T##_DefineMeta T##_DefineMeta::_reg;                                                       \
    }

#define IG_DEFINE_ARCHETYPE_META(T)                                                             \
    namespace details::meta                                                                     \
    {                                                                                           \
        T##_DefineMeta::T##_DefineMeta()                                                        \
        {                                                                                       \
            static_assert(std::is_same_v<decltype(&T::Create), ig::Entity (*)(ig::Registry*)>); \
            entt::meta<T>().type(ig::TypeHash<T>);                                              \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                              \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase());       \
            entt::meta<T>().prop(ig::meta::ArchetypeProperty);                                  \
            entt::meta<T>().template func<&T::Create>(ig::meta::ArchetypeCreateFunc);           \
        }                                                                                       \
        T##_DefineMeta T##_DefineMeta::_reg;                                                    \
    }

#define IG_DEFINE_STARTUP_APP_META(T)                                                     \
    namespace details::meta                                                               \
    {                                                                                     \
        T##_DefineMeta::T##_DefineMeta()                                                  \
        {                                                                                 \
            static_assert(std::is_base_of_v<class ig::Application, T>);                   \
            IG_CHECK(!entt::resolve(StartupAppClass));                                    \
            entt::meta<T>().type(StartupAppClass);                                        \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                        \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase()); \
            DefineMeta<T>();                                                              \
        }                                                                                 \
        T##_DefineMeta T##_DefineMeta::_reg;                                              \
    }

#define IG_SET_ON_INSPECTOR_META(T, Func)                                                \
    static_assert(std::is_invocable_v<decltype(Func), ig::Registry*, const ig::Entity>); \
    entt::meta<T>().func<&Func>(ig::meta::OnInspectorFunc)
