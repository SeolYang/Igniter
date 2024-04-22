#pragma once
#include <Igniter.h>

namespace ig
{
    template <typename Ty>
    struct Handle_New
    {
    public:
        [[nodiscard]] bool operator==(const Handle_New rhs) const noexcept { return Value == rhs.Value; }

    public:
        constexpr static uint64_t InvalidValue{std::numeric_limits<uint64_t>::max()};
        uint64_t                  Value{InvalidValue};
    };
}

template <typename Ty>
struct std::hash<ig::Handle_New<Ty>> final
{
public:
    size_t operator()(const ig::Handle_New<Ty>& handle) const noexcept
    {
        return handle.Value;
    }
};
