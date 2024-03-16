#pragma once
#pragma warning(push)
#pragma warning(disable : 26827)
#include <entt/entt.hpp>
#pragma warning(pop)
#include <Core/Container.h>

namespace fe
{
    using Entity = entt::entity;
    using Registry = entt::registry;
} // namespace fe