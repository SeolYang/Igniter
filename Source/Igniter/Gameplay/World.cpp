#include <Igniter.h>
#include <Core/Handle.h>
#include <Core/Serialization.h>
#include <Core/Meta.h>
#include <Core/String.h>
#include <Gameplay/World.h>

namespace ig
{
    Json& World::Serialize(Json& archive) const
    {
        Json entitesRoot{};
        auto resolvedMeta = entt::resolve();
        for (const auto entity : registry.view<Entity>(entt::exclude<NonSerializable>))
        {
            if (!registry.all_of<NonSerializable>(entity))
            {
                Json entityRoot{};
                for (auto&& [typeID, type] : resolvedMeta)
                {
                    const entt::sparse_set* const storage = registry.storage(typeID);
                    if (storage != nullptr && storage->contains(entity))
                    {
                        Json componentRoot{};
                        const auto nameProperty = type.prop(meta::NameProperty);
                        IG_CHECK(nameProperty);
                        componentRoot[ComponentNameHintKey] = nameProperty.value().cast<String>();
                        auto serializeJson = type.func(meta::SerializeComponentJsonFunc);
                        if (serializeJson)
                        {
                            serializeJson.invoke(type, std::ref(componentRoot), std::cref(registry), entity);
                        }

                        entityRoot[std::format("{}", typeID)] = componentRoot;
                    }
                }

                entitesRoot[std::format("{}", entt::to_integral(entity))] = entityRoot;
            }

            archive[EntitiesDataKey] = entitesRoot;
        }

        return archive;
    }

    const Json& World::Deserialize(const Json& archive)
    {
        // #sy_note Registry가 비어있을 때만 호출되나?
        if (!archive.contains(EntitiesDataKey))
        {
            return archive;
        }

        const Json& entitesRoot = archive[EntitiesDataKey];
        for (auto rootItr = entitesRoot.cbegin(); rootItr != entitesRoot.cend(); ++rootItr)
        {
            /* #sy_todo key가 number가 아니였을 경우 처리 */
            const auto oldEntity = static_cast<Entity>(std::stoul(rootItr.key()));
            const auto entity = registry.create(oldEntity);
            IG_CHECK(oldEntity == entity); /* #sy_note 버그는 아니지만, 처리해야함 */
            const Json& entityRoot = rootItr.value();
            for (auto entityItr = entityRoot.cbegin(); entityItr != entityRoot.cend(); ++entityItr)
            {
                const auto componentID = static_cast<entt::id_type>(std::stoul(entityItr.key()));
                auto resolvedType = entt::resolve(componentID);
                if (!resolvedType)
                {
                    // #sy_todo add error message
                    continue;
                }

                auto addComponent = resolvedType.func(meta::AddComponentFunc);
                if (!addComponent)
                {
                    // #sy_todo add error message
                    continue;
                }

                if (!addComponent.invoke(resolvedType, std::ref(registry), entity).cast<bool>())
                {
                    // #sy_todo add error message
                    continue;
                }

                auto deserializeJson = resolvedType.func(meta::DeserializeComponentJsonFunc);
                if (!deserializeJson)
                {
                    // #sy_todo add err message
                    continue;
                }

                deserializeJson.invoke(resolvedType, std::cref(entityItr.value()), std::ref(registry), entity);
            }
        }

        return archive;
    }
}    // namespace ig