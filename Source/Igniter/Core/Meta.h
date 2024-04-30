#pragma once
#include <Igniter.h>
#include <Core/Hash.h>
#include <Core/String.h>

namespace ig::meta
{
    constexpr inline entt::hashed_string NameProperty = "Name"_hs;
    constexpr inline entt::hashed_string TitleCaseNameProperty = "TitleCaseName"_hs;
    constexpr inline entt::hashed_string DescriptionProperty = "Description"_hs;
    constexpr inline entt::hashed_string ComponentProperty = "Component"_hs;
    constexpr inline entt::hashed_string AddComponentFunc = "AddComponent"_hs;
    constexpr inline entt::hashed_string RemoveComponentFunc = "RemoveComponent"_hs;
    constexpr inline entt::hashed_string OnInspectorFunc = "OnInspectorFunc"_hs;    // Signature:OnInspectorFunc(Registry*, Entity);
}    // namespace ig::meta

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
}    // namespace ig

#define IG_DECLARE_TYPE_META(T)         \
    template <typename Ty>              \
    void DefineMeta();                  \
    namespace details::meta             \
    {                                   \
        struct T##_DefineMeta           \
        {                               \
            T##_DefineMeta();           \
                                        \
        private:                        \
            static T##_DefineMeta _reg; \
        };                              \
    }

#define IG_DEFINE_TYPE_META(T)                                                            \
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

#define IG_DEFINE_TYPE_META_AS_COMPONENT(T)                                                        \
    namespace details::meta                                                                        \
    {                                                                                              \
        T##_DefineMeta::T##_DefineMeta()                                                           \
        {                                                                                          \
            entt::meta<T>().type(TypeHash<T>);                                                     \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                                 \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase());          \
            entt::meta<T>().prop(ig::meta::ComponentProperty);                                     \
            entt::meta<T>().template func<&ig::AddComponent<T>>(ig::meta::AddComponentFunc);       \
            entt::meta<T>().template func<&ig::RemoveComponent<T>>(ig::meta::RemoveComponentFunc); \
            DefineMeta<T>();                                                                       \
        }                                                                                          \
        T##_DefineMeta T##_DefineMeta::_reg;                                                       \
    }

#define IG_SET_META_ON_INSPECTOR_FUNC(T, Func)                                           \
    static_assert(std::is_invocable_v<decltype(Func), ig::Registry*, const ig::Entity>); \
    entt::meta<T>().func<&Func>(ig::meta::OnInspectorFunc)
