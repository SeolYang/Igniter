#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/Meta.h"
#include "Igniter/Core/String.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/World.h"

IG_DEFINE_LOG_CATEGORY(World);

namespace ig
{
    World::World(AssetManager& assetManager, const Handle<Map, AssetManager> map) : assetManager(&assetManager), map(map)
    {
        if (map)
        {
            const Map* mapPtr = assetManager.Lookup(map);
            if (mapPtr != nullptr)
            {
                Deserialize(mapPtr->GetSerializedWorld());
            }
        }
    }

     World::~World() 
     {
         if (assetManager != nullptr && !map.IsNull())
         {
            assetManager->Unload(map);
         }
     }

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
                const auto nameProperty = resolvedType.prop(meta::NameProperty);
                if (!resolvedType)
                {
                    IG_LOG(World, Warning, "Ignored Component {}({}): {} does not registered to meta registry.", nameProperty.value().cast<String>(),
                        componentID);
                    continue;
                }

                auto addComponent = resolvedType.func(meta::AddComponentFunc);
                if (!addComponent)
                {
                    IG_LOG(World, Warning, "Ignored Type ID: {} add component meta function does not registered to meta registry.",
                        nameProperty.value().cast<String>());
                    continue;
                }

                if (!addComponent.invoke(resolvedType, std::ref(registry), entity).cast<bool>())
                {
                    IG_LOG(World, Warning, "Failed to add component {}({}) to entity", nameProperty.value().cast<String>(), componentID);
                    continue;
                }

                auto deserializeJson = resolvedType.func(meta::DeserializeComponentJsonFunc);
                if (!deserializeJson)
                {
                    IG_LOG(World, Warning, "Ignored component {}({}) deserialization. The deserialize meta function does not registered.",
                        nameProperty.value().cast<String>(), componentID);
                    continue;
                }

                deserializeJson.invoke(resolvedType, std::cref(entityItr.value()), std::ref(registry), entity);
            }
        }

        return archive;
    }
}    // namespace ig