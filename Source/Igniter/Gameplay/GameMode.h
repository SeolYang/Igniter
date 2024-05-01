#pragma once
#include "Igniter/Core/Meta.h"

namespace ig::meta
{
    constexpr inline entt::hashed_string GameModeProperty = "GameMode"_hs;
    constexpr inline entt::hashed_string CreateGameModeFunc = "CreateGameMode"_hs;
}    // namespace ig::meta

namespace ig
{
    class World;
    class GameMode
    {
    public:
        GameMode() = default;
        GameMode(const GameMode&) = delete;
        GameMode(GameMode&&) noexcept = delete;
        virtual ~GameMode() = default;

        GameMode& operator=(const GameMode&) = delete;
        GameMode& operator=(GameMode&&) noexcept = delete;

        virtual void Update(World& world) = 0;
    };

    template <typename Ty>
        requires std::is_base_of_v<GameMode, Ty>
    Ptr<GameMode> CreateGameMode()
    {
        return MakePtr<Ty>();
    }
}    // namespace ig

#define IG_DEFINE_TYPE_META_AS_GAME_MODE(T)                                               \
    namespace details::meta                                                               \
    {                                                                                     \
        T##_DefineMeta::T##_DefineMeta()                                                  \
        {                                                                                 \
            entt::meta<T>().type(TypeHash<T>);                                            \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                        \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase()); \
            entt::meta<T>().prop(ig::meta::GameModeProperty);                             \
            entt::meta<T>().func<&ig::CreateGameMode<T>>(ig::meta::CreateGameModeFunc);   \
            DefineMeta<T>();                                                              \
        }                                                                                 \
        T##_DefineMeta T##_DefineMeta::_reg;                                              \
    }
