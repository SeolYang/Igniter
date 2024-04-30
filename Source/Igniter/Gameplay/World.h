#pragma once
#include <Igniter.h>
#include <Core/Handle.h>

namespace ig
{
    class World final
    {
    public:
        World() = default;
        World(const World&) = delete;
        World(World&&) noexcept = default;
        ~World() = default;

        World& operator=(const World&) = delete;
        World& operator=(World&&) noexcept = default;

        Registry& GetRegistry() { return registry; }

        /* #sy_todo 프로토타이핑 했던 거에 기반해서 구현(런타임 메타 데이타 참고) */
        Json& Serialize(Json& archive);
        const Json& Deserialize(const Json& archive);

    private:
        Registry registry{};
    };
}    // namespace ig
