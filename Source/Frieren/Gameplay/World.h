#pragma once
#include <entt/entt.hpp>

namespace fe
{
	using Entity = entt::entity;
	class World
	{
	public:
		using Registry = entt::registry;

		World() = default;
		World(const World&) = delete;
		World(World&&) noexcept = delete;

		World& operator=(const World&) = delete;
		World& operator=(World&&) noexcept = delete;

		[[nodiscard]] inline Entity Create()
		{
			return registry.create();
		}

		void Destroy(Entity entity);
		template <typename Itr>
		void Destroy(Itr begin, const Itr end)
		{
			for (; begin != end; ++begin)
			{
				Destroy(*begin);
			}
		}

		template <typename Component>
		[[nodiscard]] Entity ToEntity(const Component& component) const
		{
			return entt::to_entity(registry, component);
		}

		template <typename Component, typename... Args>
		auto Attach(const Entity entity, [[maybe_unused]] Args&&... args)
		{
			return registry.emplace<Component>(entity, std::forward<Args>(args)...);
		}

		template <typename Component, typename Itr>
		void Attach(Itr begin, Itr end, const Component& value = {})
		{
			registry.insert<Component>(begin, end, value);
		}

		template <typename Component, typename... Args>
		auto AttachOrReplace(const Entity entity, Args&&... args)
		{
			if (registry.all_of<Component>(entity))
			{
				return registry.replace<Component>(entity, std::forward<Args>(args)...);
			}
			else
			{
				return registry.emplace<Component>(entity, std::forward<Args>(args)...);
			}
		}

		template <typename... Components>
		void Detach(const Entity entity)
		{
			registry.erase<Components...>(entity);
		}

		template <typename... Components, typename Itr>
		void Detach(Itr begin, Itr end)
		{
			registry.erase<Components...>(begin, end);
		}

		template <typename... Components>
		size_t SafeDetach(const Entity entity)
		{
			return registry.remove<Components...>(entity);
		}

		template <typename... Components, typename Itr>
		size_t SafeDetach(Itr begin, Itr end)
		{
			return registry.remove<Components...>(begin, end);
		}

		template <typename Component, typename... Args>
		Component& Replace(const Entity entity, Args&&... args)
		{
			return registry.emplace<Component>(entity, std::forward<Args>(args)...);
		}

		template <typename Component, typename... Func>
		Component& Patch(const Entity entity, Func&&... func)
		{
			return registry.patch<Component>(entity, std::forward<Func>(func)...);
		}

		template <typename... Components>
		[[nodiscard]] auto Get(const Entity entity)
		{
			return registry.get<Components...>(entity);
		}

		template <typename... Components>
		[[nodiscard]] auto Get(const Entity entity) const
		{
			return registry.get<Components...>(entity);
		}

		template <typename... Components, typename... Excludes, typename Func>
		void Each(Func func, [[maybe_unused]] Excludes... excludes)
		{
			registry.view<Components...>(excludes...).each(func);
		}

		template <typename... Components, typename... Excludes, typename Func>
		void Each(Func func, [[maybe_unused]] Excludes... excludes) const
		{
			registry.view<Components...>(excludes...).each(func);
		}

		template <typename... Components, typename... Excludes>
		[[nodiscard]] auto View([[maybe_unused]] Excludes... excludes)
		{
			return registry.view<Components...>(excludes...);
		}

		template <typename... Components, typename... Excludes>
		[[nodiscard]] auto View([[maybe_unused]] Excludes... excludes) const
		{
			return registry.view<Components...>(excludes...);
		}

		template <typename... Components>
		void Clear()
		{
			registry.clear<Components...>();
		}

		void Reset()
		{
			registry.clear();
		}

		template <typename... Components>
		[[nodiscard]] bool AllOf(const Entity entity) const
		{
			return registry.all_of<Components...>(entity);
		}

		template <typename... Components>
		[[nodiscard]] bool AnyOf(const Entity entity) const
		{
			return registry.any_of<Components...>(entity);
		}

		/** (numOfComponents in entity == 0) == true */
		[[nodiscard]] bool IsOrphan(const Entity entity) const;
		/** Entity does not destroyed. */
		[[nodiscard]] bool IsValid(const Entity entity) const;

	private:
		Registry registry;
	};
} // namespace fe