#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    template <typename... Ts>
    struct ComponentClassifier;

    template <typename T>
    struct ComponentClassifier<T>
    {
        using TagComponents = std::conditional_t<
            std::is_empty_v<T>,
            std::tuple<T>,
            std::tuple<>>;
        using Components = std::conditional_t<
            std::is_empty_v<T>,
            std::tuple<>,
            std::tuple<T>>;
    };

    template <typename T, typename... Ts>
    struct ComponentClassifier<T, Ts...>
    {
    private:
        using Rest = ComponentClassifier<Ts...>;

    public:
        using TagComponents = std::conditional_t<
            std::is_empty_v<T>,
            decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<typename Rest::TagComponents>())),
            typename Rest::TagComponents>;

        using Components = std::conditional_t<
            std::is_empty_v<T>,
            typename Rest::Components,
            decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<typename Rest::Components>()))>;
    };

    template <typename... Components>
    struct Archetype
    {
    public:
        using ClassifiedComponents = ComponentClassifier<Components...>;
        constexpr static bool bHasTagComponents = !std::is_same_v<typename ClassifiedComponents::TagComponents, std::tuple<>>;
        
    public:
        static Entity Create(Registry* registryPtr)
        {
            if (registryPtr == nullptr)
            {
                return NullEntity;
            }

            const Entity newEntity = registryPtr->create();
            CreateInternal(registryPtr, newEntity);
            return newEntity;
        }

        static Entity CreateRef(Registry& registry)
        {
            return Create(&registry);
        }

        static bool IsArchetype(const Entity entity, const Registry& registry)
        {
            return registry.all_of<Components...>(entity);
        }

        static auto View(Registry& registry)
        {
            return registry.view<Components...>();
        }

        static auto ConstView(const Registry& registry)
        {
            return registry.view<Components...>();
        }

    private:
        template <typename... Ts>
        static void EmplaceTagInternal(Registry* registry, const Entity entity, [[maybe_unused]] const std::tuple<Ts...>& tagComponentTuple)
        {
            (registry->emplace<Ts>(entity), ...);
        }

        template <typename... Ts>
        static auto EmplaceInternal(Registry* registry, const Entity entity, [[maybe_unused]] const std::tuple<Ts...>& componentTuple)
        {
            return std::tie(registry->emplace<Ts>(entity)...);
        }

        static auto CreateInternal(Registry* registry, const Entity newEntity)
        {
            IG_CHECK(regisry != nullptr);
            IG_CHECK(newEntity != NullEntity);
            if constexpr (std::tuple_size_v<typename ClassifiedComponents::TagComponents> > 0)
            {
                static const auto kTagComponentsTuple = typename ClassifiedComponents::TagComponents();
                EmplaceTagInternal(registry, newEntity, kTagComponentsTuple);
            }
            static_assert(std::tuple_size_v<typename ClassifiedComponents::Components> > 0);
            static const auto kComponentsTuple = typename ClassifiedComponents::Components();
            return EmplaceInternal(registry, newEntity, kComponentsTuple);
        }
    };
} // namespace ig
