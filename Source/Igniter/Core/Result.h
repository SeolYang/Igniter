#pragma once
#include <concepts>
#include <memory>
#include <Core/Assert.h>

namespace ig
{
    namespace details
    {
        struct NonTrivialDummy
        {
            constexpr NonTrivialDummy() noexcept {}
        };
    } // namespace details

    template <typename T>
    concept ResultStatus = requires {
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

    class BadResultAccess : public std::exception
    {
    public:
        [[nodiscard]] const char* what() const noexcept override { return "Bad Result Access"; }
    };

    /* 일반적인 Error Code 의 확장 버전. */
    template <typename T, ResultStatus E>
        requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
    class Result final
    {
    public:
        Result(const Result&) = delete;
        Result(Result&& other) noexcept = delete;
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

        Result& operator=(const Result&) = delete;
        Result& operator=(Result&&) noexcept = delete;

        [[nodiscard]] T Take()
        {
            if (!HasOwnership())
            {
                throw BadResultAccess();
            }

            bOwnershipTransferred = true;
            if constexpr (std::is_trivial_v<T>)
            {
                return std::exchange(value, {});
            }
            else
            {
                return std::move(value);
            }
        }

        [[nodiscard]] bool HasOwnership() const noexcept { return IsSuccess() && !bOwnershipTransferred; }
        [[nodiscard]] bool IsSuccess() const noexcept { return status == E::Success; }
        [[nodiscard]] E GetStatus() const noexcept { return status; }

    private:
        template <typename Ty, ResultStatus En, typename... Args>
        friend Result<Ty, En> MakeSuccess(Args&&... args);

        template <typename Ty, ResultStatus En, En Status>
            requires(Status != En::Success)
        friend Result<Ty, En> MakeFail();

        template <typename... Args>
        Result(const E newStatus, Args&&... args) : status(newStatus)
        {
            if constexpr (std::is_trivial_v<T>)
            {
                value = {};
            }
            else
            {
                dummy = {};
            }

            if (newStatus == E::Success)
            {
                ::new (&value) T(std::forward<Args>(args)...);
            }
        }

    private:
        union
        {
            details::NonTrivialDummy dummy;
            std::decay_t<T> value;
        };

        E status;
        bool bOwnershipTransferred = false;
    };

    template <typename T, ResultStatus E, typename... Args>
    Result<T, E> MakeSuccess(Args&&... args)
    {
        /* prvalue -> Guaranteed Copy Elision */
        return Result<T, E>{ E::Success, std::forward<Args>(args)... };
    }

    template <typename T, ResultStatus E, E Status>
        requires(Status != E::Success)
    Result<T, E> MakeFail()
    {
        /* prvalue -> Guaranteed Copy Elision */
        return Result<T, E>{ Status };
    }

} // namespace ig
