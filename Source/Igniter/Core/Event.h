#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    /* 호출되는 델리게이트 들은 각자의 스레드 안정성을 스스로 보장해야 한다. */
    template <typename Identifier, typename... Params>
        requires(std::is_object_v<Identifier>) && (std::is_object_v<Params> && ...)
    class [[nodiscard]] Event final
    {
        using DelegateType = std::function<void(Params...)>;
        using SubscriberType = std::pair<Identifier, DelegateType>;

      public:
        Event() = default;
        Event(const Event&) = default;
        Event(Event&&) noexcept = default;
        ~Event() = default;

        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) noexcept = default;

        template <typename I, typename D>
            requires std::is_convertible_v<I, Identifier> && std::is_convertible_v<D, DelegateType>
        void Subscribe(I&& id, D&& delegate)
        {
            subscribeTable.insert_or_assign(std::forward<I>(id), std::forward<D>(delegate));
        }

        void Unsubscribe(const Identifier& id) { subscribeTable.erase(id); }

        [[nodiscard]] bool HasAnySubscriber() const { return !subscribeTable.empty(); }

        [[nodiscard]] bool HasSubscriber(const Identifier& id) const { return subscribeTable.contains(id); }

        void Notify(const Params&... args)
        {
            for (auto& subscribe : subscribeTable)
            {
                subscribe.second(args...);
            }
        }

        void Clear() { subscribeTable.clear(); }

      private:
        UnorderedMap<Identifier, DelegateType> subscribeTable{};
    };
} // namespace ig
