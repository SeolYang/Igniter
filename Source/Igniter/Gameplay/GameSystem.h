#pragma once
#include "Igniter/Core/Meta.h"

namespace ig::meta
{
    constexpr inline entt::hashed_string GameSystemProperty      = "GameSystem"_hs;
    constexpr inline entt::hashed_string GameSystemConstructFunc = "GameSystemConstruct"_hs;
} // namespace ig::meta

namespace ig
{
    class World;
    /*
     * #sy_note 게임 시스템은 간단한 작업을 하는 단일 시스템 일수도 있고. 여러 시스템이 합쳐져 좀 더 복잡한 일 들을 처리하는
     * 복합 시스템 일수도 있다. 대부분의 경우 GameSystem 인터페이스를 구현하는 경우, 인-게임 로직을 표현 하기 위해서가 대부분 이지만.
     * 그렇지 않고 좀 더 복잡한 연산들을 하는데 사용되어도 문제 없다.
     */
    class GameSystem
    {
    public:
        GameSystem()                      = default;
        GameSystem(const GameSystem&)     = delete;
        GameSystem(GameSystem&&) noexcept = delete;
        virtual ~GameSystem()             = default;

        GameSystem& operator=(const GameSystem&)     = delete;
        GameSystem& operator=(GameSystem&&) noexcept = delete;

        virtual void PreUpdate([[maybe_unused]] const float deltaTime, [[maybe_unused]] World& world) { }
        virtual void Update(const float deltaTime, World& world) = 0;
        virtual void PostUpdate([[maybe_unused]] const float deltaTime, [[maybe_unused]] World& world) { }
    };

    template <typename Ty>
        requires std::is_base_of_v<GameSystem, Ty>
    Ptr<GameSystem> CreateGameSystem()
    {
        return MakePtr<Ty>();
    }
} // namespace ig

#define IG_DEFINE_TYPE_META_AS_GAME_SYSTEM(T)                                                  \
    namespace details::meta                                                                    \
    {                                                                                          \
        T##_DefineMeta::T##_DefineMeta()                                                       \
        {                                                                                      \
            entt::meta<T>().type(ig::TypeHash<T>);                                             \
            entt::meta<T>().prop(ig::meta::NameProperty, #T##_fs);                             \
            entt::meta<T>().prop(ig::meta::TitleCaseNameProperty, #T##_fs.ToTitleCase());      \
            entt::meta<T>().prop(ig::meta::GameSystemProperty);                                \
            entt::meta<T>().func<&ig::CreateGameSystem<T>>(ig::meta::GameSystemConstructFunc); \
            DefineMeta<T>();                                                                   \
        }                                                                                      \
        T##_DefineMeta T##_DefineMeta::_reg;                                                   \
    }
