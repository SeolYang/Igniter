#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"

namespace ig
{
    struct NonSerializable
    {
    };

    class Map;
    class AssetManager;

    class World
    {
      public:
        World() = default;
        World(AssetManager& assetManager, const Handle<Map, AssetManager> map);
        World(const World&) = delete;
        World(World&&) noexcept = default;
        virtual ~World();

        World& operator=(const World&) = delete;
        World& operator=(World&&) noexcept = default;

        [[nodiscard]] Registry& GetRegistry() noexcept { return registry; }
        [[nodiscard]] const Registry& GetRegistry() const noexcept { return registry; }

        /* #sy_todo 프로토타이핑 했던 거에 기반해서 구현(런타임 메타 데이타 참고) */
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

      private:
        constexpr static std::string_view EntitiesDataKey = "Entities";
        constexpr static std::string_view ComponentNameHintKey = "NameHint";

        AssetManager* assetManager = nullptr;
        Handle<Map, AssetManager> map{};
        Registry registry{};
    };
} // namespace ig
