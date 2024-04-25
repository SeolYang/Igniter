#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename Ty>
    struct Handle final
    {
    public:
        Handle() noexcept = default;
        Handle(const uint64_t newValue) : Value(newValue) {}
        Handle(const Handle&) noexcept = default;
        Handle(Handle&& other) noexcept 
            : Value(std::exchange(other.Value, InvalidValue))
        {
        }
        ~Handle() = default;

        Handle& operator=(const Handle&) noexcept = default;
        Handle& operator=(Handle&& other) noexcept 
        {
            Value = std::exchange(other.Value, InvalidValue);
            return *this;
        }

        [[nodiscard]] bool operator==(const Handle rhs) const noexcept { return Value == rhs.Value; }
        [[nodiscard]] operator bool() const noexcept { return Value != InvalidValue; }

    public:
        constexpr static uint64_t InvalidValue{std::numeric_limits<uint64_t>::max()};
        uint64_t Value{InvalidValue};
    };
}    // namespace ig

template <typename Ty>
struct std::hash<ig::Handle<Ty>> final
{
public:
    size_t operator()(const ig::Handle<Ty>& handle) const noexcept { return handle.Value; }
};
