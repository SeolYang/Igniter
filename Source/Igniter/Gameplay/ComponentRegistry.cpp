#include <Igniter.h>
#include <Core/Engine.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    void ComponentRegistry::Register(const entt::id_type componentTypeID, const ComponentInfo componentInfo)
    {
        if (Igniter::IsInitialized())
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        static robin_hood::unordered_set<entt::id_type> componentIDSet{};
        if (componentIDSet.contains(componentTypeID))
        {
            IG_CHECK_NO_ENTRY();
            return;
        }

        auto& componentInfos = GetComponentInfosInternal();
        componentInfos.emplace_back(componentTypeID, componentInfo);
        componentIDSet.insert(componentTypeID);
    }

    std::vector<std::pair<entt::id_type, ComponentRegistry::ComponentInfo>>& ComponentRegistry::GetComponentInfosInternal()
    {
        static std::vector<std::pair<entt::id_type, ComponentRegistry::ComponentInfo>> componentInfoInstance{};
        return componentInfoInstance;
    }

    std::span<const std::pair<entt::id_type, ComponentRegistry::ComponentInfo>> ComponentRegistry::GetComponentInfos()
    {
        static std::once_flag sorted{};
        auto& componentInfos = GetComponentInfosInternal();
        std::call_once(sorted,
                       [&componentInfos]()
                       {
                           std::sort(componentInfos.begin(), componentInfos.end(),
                                     [](const auto& lhs, const auto& rhs)
                                     {
                                         return lhs.second.Name < rhs.second.Name;
                                     });
                       });

        return componentInfos;
    }
} // namespace ig