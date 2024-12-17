#pragma once
#include "Igniter/Igniter.h"

template <typename Enumeration>
    requires std::is_enum_v<Enumeration>
struct std::formatter<Enumeration>
{
public:
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FrameContext>
    auto format(const Enumeration& enumerator, FrameContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", magic_enum::enum_name(enumerator));
    }
};

template <>
struct std::formatter<ig::Guid>
{
public:
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FrameContext>
    auto format(const ig::Guid& guid, FrameContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", (guid.isValid() ? guid.str() : "#INVALID_GUID"));
    }
};