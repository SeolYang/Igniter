#pragma once
#include <optional>

namespace ig
{
    template <typename ResultEnum, ResultEnum Succeeded, typename T>
    struct Result
    {
    public:
        template <typename... Args>
        Result(const ResultEnum result, Args&&... args)
            : value(std::make_optional<T>(std::forward<Args>(args)...))
        {
        }

        bool IsSucceeded() const { return value && result == Succeeded; }

        T MoveValue()
        {
            IG_CHECK(IsSucceeded());
            return std::move(*value);
        }

    private:
        std::optional<T> value = std::nullopt;
        ResultEnum result;
    };
} // namespace ig
