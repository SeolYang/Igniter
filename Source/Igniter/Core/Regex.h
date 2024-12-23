#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"

namespace ig
{
    [[nodiscard]] bool RegexMatch(
        const String str, const std::regex& regex, const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
    [[nodiscard]] std::vector<String> RegexMatchN(
        const String str, const std::regex& regex, const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
    [[nodiscard]] std::vector<String> RegexSearch(const String str, const std::regex& regex);
    [[nodiscard]] String              RegexReplace(const String                   str, const std::regex& regex, const String replacePattern,
                                      const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
} // namespace ig
