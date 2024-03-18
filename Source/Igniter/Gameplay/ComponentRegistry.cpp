#include <Core/Assert.h>
#include <Core/Igniter.h>
#include <Gameplay/ComponentRegistry.h>

namespace ig
{
    std::vector<std::pair<entt::id_type, ig::ComponentRegistry::ComponentInfo>> ComponentRegistry::componentInfos;

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

        componentInfos.emplace_back(componentTypeID, componentInfo);
        componentIDSet.insert(componentTypeID);
    }

    std::span<const std::pair<entt::id_type, ComponentRegistry::ComponentInfo>> ComponentRegistry::GetComponentInfos()
    {
        static std::once_flag sorted{};
        std::call_once(sorted,
                       []()
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