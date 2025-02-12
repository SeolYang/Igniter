#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    template <typename Ty>
    struct Handle final
    {
      public:
        Handle() noexcept = default;
        explicit Handle(const uint64_t newValue)
            : Value(newValue)
        {
        }
        Handle(const Handle&) noexcept = default;
        Handle(Handle&& other) noexcept
            : Value(std::exchange(other.Value, NullValue))
        {
        }
        ~Handle() = default;

        Handle& operator=(const Handle&) noexcept = default;

        Handle& operator=(Handle&& other) noexcept
        {
            Value = std::exchange(other.Value, NullValue);
            return *this;
        }

        [[nodiscard]] bool operator==(const Handle rhs) const noexcept { return Value == rhs.Value; }
        [[nodiscard]] operator bool() const noexcept { return Value != NullValue; }
        [[nodiscard]] bool IsNull() const noexcept { return Value == NullValue; }
        [[nodiscard]] U64 GetHash() const noexcept { return Value; }

      private:
        constexpr static uint64_t NullValue{std::numeric_limits<uint64_t>::max()};

      public:
        uint64_t Value{NullValue};
    };
} // namespace ig

template <typename Ty>
struct std::hash<ig::Handle<Ty>>
{
  public:
    ig::Size operator()(const ig::Handle<Ty>& handle) const noexcept { return handle.GetHash(); }
};
