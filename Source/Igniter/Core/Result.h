#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    template <typename T>
    concept ResultStatus = requires
    {
        {
            T::Success
        };

        {
            std::is_enum_v<T>
        };
    };

    template <ResultStatus E>
    constexpr bool IsSucceeded(const E status)
    {
        return status == E::Success;
    }

    /* 일반적인 Error Code 의 확장 버전. */
    template <typename T, ResultStatus E>
        requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
    class [[nodiscard]] Result final
    {
    public:
        Result(const Result&) = delete;

        Result(Result&& other) noexcept : dummy{ }, status(other.status)
        {
            IG_CHECK(other.status != E::Success && other.bOwnershipTransferred || other.status == E::Success && !other.bOwnershipTransferred);
            if (!other.bOwnershipTransferred)
            {
                ::new(&value) T(std::move(other.value));
            }
            bOwnershipTransferred = std::exchange(other.bOwnershipTransferred, true);
        }

        ~Result()
        {
            IG_CHECK(!IsSuccess() || (IsSuccess() && bOwnershipTransferred) && "Result doesn't handled.");
            if constexpr (!std::is_trivial_v<T>)
            {
                if (status == E::Success)
                {
                    value.~T();
                }
            }
        }

        Result& operator=(const Result&)     = delete;
        Result& operator=(Result&&) noexcept = delete;

        [[nodiscard]] T Take()
        {
            IG_CHECK(HasOwnership());

            bOwnershipTransferred = true;
            if constexpr (std::is_trivial_v<T>)
            {
                return std::exchange(value, { });
            }
            else
            {
                return std::move(value);
            }
        }

        [[nodiscard]] bool HasOwnership() const noexcept { return IsSuccess() && !bOwnershipTransferred; }
        [[nodiscard]] bool IsSuccess() const noexcept { return status == E::Success; }
        [[nodiscard]] E    GetStatus() const noexcept { return status; }

    private:
        template <typename Ty, ResultStatus En, typename... Args>
        friend Result<Ty, En> MakeSuccess(Args&&... args);

        template <typename T, auto Status>
            requires(ResultStatus<decltype(Status)> && Status != decltype(Status)::Success)
        friend Result<T, decltype(Status)> MakeFail();

        Result(const E newStatus) : dummy{ }, status(newStatus) { }

        template <typename... Args>
        Result(Args&&... args) : dummy{ }, status(E::Success), bOwnershipTransferred(false)
        {
            ::new(&value) T(std::forward<Args>(args)...);
        }

    private:
        union
        {
            struct NonTrivialDummy
            {
                constexpr NonTrivialDummy() noexcept { }
            } dummy;

            std::decay_t<T> value;
        };

        const E status;
        bool    bOwnershipTransferred = true;
    };

    template <typename T, ResultStatus E, typename... Args>
    Result<T, E> MakeSuccess(Args&&... args)
    {
        return Result<T, E>{std::forward<Args>(args)...};
    }

    template <typename T, auto Status>
        requires(ResultStatus<decltype(Status)> && Status != decltype(Status)::Success)
    Result<T, decltype(Status)> MakeFail()
    {
        return Result<T, decltype(Status)>{Status};
    }
} // namespace ig
