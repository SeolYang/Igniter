#pragma once
#include <concepts>
#include <memory>
#include <Core/Assert.h>

/* #sy_todo */
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

    /* 일반적인 Error Code 의 확장 버전. 발생한 결과물은 성공하든 실패하든 발생 함수의 호출자에 의해 처리되는 것을 상정 하였음. */
    template <ResultStatus StatusType, typename Ty>
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
            std::remove_cv_t<Ty> value;
        };

        StatusType status;
        bool bOwnershipTransferred = false;
    };

    template <ResultStatus StatusType, typename Ty, StatusType Status>
        requires(Status != StatusType::Success)
    Result<StatusType, Ty> MakeFail()
    {
        return Result<StatusType, Ty>::template Make<Status>();
    }

    template <ResultStatus StatusType, typename Ty, typename... Args>
    Result<StatusType, Ty> MakeSuccess(Args&&... args)
    {
        return Result<StatusType, Ty>::template Make<StatusType::Success>(std::forward<Args>(args)...);
    }
} // namespace ig
