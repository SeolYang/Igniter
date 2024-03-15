#pragma once
#include <functional>
#include <concepts>
#include <Core/Container.h>
#include <Core/String.h>
#include <Core/Assert.h>

namespace fe
{
    /**
     * Does not guarantee thread-safety. (The event subscribe/unsubscribe and call of notify. And also callee it self. )
     */
    template <typename Identifier = fe::String, typename... Params>
        requires(std::is_object_v<Identifier>) && (std::is_object_v<Params> && ...)
    class Event
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
        void Subscribe(I&& id, D&& delegate)
        {
            Subscribe(std::make_pair(std::forward<I>(id), std::forward<D>(delegate)));
        }

        template <typename S>
        void Subscribe(S&& subscriber)
        {
            if (!HasSubscriber(subscriber.first))
            {
                subscribers.emplace_back(std::forward<S>(subscriber));
            }
            else
            {
                checkNoEntry();
            }
        }

        void Unsubscribe(const Identifier& id)
        {
            for (auto itr = subscribers.begin(); itr != subscribers.end(); ++itr)
            {
                if (itr->first == id)
                {
                    subscribers.erase(itr);
                    return;
                }
            }
        }

        bool HasSubscriber(const Identifier& id) const
        {
            for (const auto& subscriber : subscribers)
            {
                if (subscriber.first == id)
                {
                    return true;
                }
            }

            return false;
        }

        /*
         * @warn	만약에 perfect forwarding 되버려서 임의의 argument 가 특정 delegate 호출로 '이동'되어 버리면,
         * 이후 호출에서 해당 argument 가 의도했던 대로 전달되지 않을 수 있다.
         */
        template <typename... Args>
        void Notify(Args&&... args)
        {
            for (auto& subscriber : subscribers)
            {
                subscriber.second(std::forward<Args>(args)...);
            }
        }

    private:
        std::vector<SubscriberType> subscribers{};
    };
} // namespace fe