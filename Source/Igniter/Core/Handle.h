#pragma once
#include <Igniter.h>

namespace ig
{
    class DefaultHandleTag;
    template <typename Ty, typename Tag = DefaultHandleTag>
    struct Handle final
    {
    public:
        Handle() noexcept = default;
        Handle(const uint64_t newValue) : Value(newValue) {}
        Handle(const Handle&) noexcept = default;
        Handle(Handle&& other) noexcept : Value(std::exchange(other.Value, NullValue)) {}
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

    private:
        constexpr static uint64_t NullValue{std::numeric_limits<uint64_t>::max()};

    public:
        uint64_t Value{NullValue};
    };
}    // namespace ig

template <typename Ty, typename Tag>
struct std::hash<ig::Handle<Ty, Tag>> final
{
public:
    size_t operator()(const ig::Handle<Ty, Tag>& handle) const noexcept { return handle.Value; }
};
