#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Regex.h"
#include "Igniter/Core/Hash.h"

namespace ig
{
    inline std::wstring Utf8ToUtf16(const std::string_view from)
    {
        std::wstring result;
        utf8::utf8to16(from.begin(), from.end(), std::back_inserter(result));
        return result;
    }

    inline std::string Utf16ToUtf8(const std::wstring_view from)
    {
        std::string result;
        utf8::utf16to8(from.begin(), from.end(), std::back_inserter(result));
        return result;
    }

    inline Vector<std::string> Split(const std::string_view target, const std::string_view delimiter)
    {
        auto splitStrViews{
            target | views::split(delimiter) |
            views::transform(
                [](auto&& element)
                {
                    return std::string_view{&*element.begin(), static_cast<Size>(ranges::distance(element))};
                })
        };

        // results의 크기를 미리 reserve 할 수 있는 방법이 없을까?
        Vector<std::string> results{};
        for (const std::string_view result : splitStrViews)
        {
            results.emplace_back(result);
        }

        return results;
    }

    inline std::string ToTitleCase(const std::string_view str)
    {
        if (str.empty())
        {
            return {};
        }
    
        static const std::regex LetterSpacingRegex{"([a-z])([A-Z])", std::regex_constants::optimize};
        static const std::regex NumberSpacingRegex{"([a-zA-Z])([0-9])", std::regex_constants::optimize};
        constexpr std::string_view kReplacePattern{"$1 $2"};
    
        std::string spacedStr = RegexReplace(str, LetterSpacingRegex, kReplacePattern);
        spacedStr = RegexReplace(spacedStr, NumberSpacingRegex, kReplacePattern);
        std::transform(spacedStr.begin(), spacedStr.begin() + 1, spacedStr.begin(), [](const char character)
        {
            return (char)std::toupper((int)character);
        });
        return spacedStr;
    }

    inline U64 Hash(const std::string_view str)
    {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }
}
