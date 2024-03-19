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

    template <ResultStatus StatusType>
    constexpr bool IsSucceeded(const StatusType status)
    {
        return status == StatusType::Success;
    }

    /* 일반적인 Error Code 의 확장 버전. */
    template <typename Ty, ResultStatus StatusType>
    class Result
    {
    public:
        Result(Result&& other) noexcept : status(other.status),
                                          bOwnershipTransferred(std::exchange(other.bOwnershipTransferred, true))
        {
            if (other.IsSuccess() && !bOwnershipTransferred)
            {
                if constexpr (std::is_trivial_v<Ty>)
                {
                    value = std::exchange(other.value, {});
                }
                else
                {
                    value = std::move(other.value);
                }
            }
        }

        ~Result()
        {
            IG_CHECK(!IsSuccess() || (IsSuccess() && bOwnershipTransferred) && "Result doesn't handled.");
            if constexpr (!std::is_trivial_v<Ty>)
            {
                if (status == StatusType::Success)
                {
                    value.~Ty();
                }
            }
        }

        [[nodiscard]] auto TakeOwnership()
        {
            IG_CHECK(status == StatusType::Success);
            IG_CHECK(!bOwnershipTransferred);
            bOwnershipTransferred = true;

            if constexpr (std::is_trivial_v<Ty>)
            {
                return std::exchange(value, {});
            }
            else
            {
                return std::move(value);
            }
        }

        [[nodiscard]] bool IsSuccess() const noexcept { return status == StatusType::Success; }
        [[nodiscard]] StatusType GetStatus() const noexcept { return status; }

        template <StatusType Status, typename... Args>
        static Result Make(Args&&... args)
        {
            Result newResult{ Status };
            if constexpr (Status == StatusType::Success)
            {
                ::new (&newResult.value) Ty(std::forward<Args>(args)...);
            }
            return newResult;
        }

    private:
        Result(const StatusType newStatus) : status(newStatus)
        {
            if constexpr (std::is_trivial_v<Ty>)
            {
                value = {};
            }
            else
            {
                dummy = {};
            }
        }

    private:
        union
        {
            details::NonTrivialDummy dummy;
            std::decay_t<Ty> value;
        };

        StatusType status;
        bool bOwnershipTransferred = false;
    };

    template <typename Ty, ResultStatus StatusType, StatusType Status>
        requires(Status != StatusType::Success)
    Result<Ty, StatusType> MakeFail()
    {
        return Result<Ty, StatusType>::template Make<Status>();
    }

    template <typename Ty, ResultStatus StatusType, typename... Args>
    Result<Ty, StatusType> MakeSuccess(Args&&... args)
    {
        return Result<Ty, StatusType>::template Make<StatusType::Success>(std::forward<Args>(args)...);
    }
} // namespace ig
