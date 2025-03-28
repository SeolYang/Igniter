#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    [[nodiscard]] bool RegexMatch(
        std::string_view str, const std::regex& regex, const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
    [[nodiscard]] Vector<std::string> RegexMatchN(
        std::string_view str, const std::regex& regex, const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
    [[nodiscard]] Vector<std::string> RegexSearch(std::string_view str, const std::regex& regex);
    [[nodiscard]] std::string RegexReplace(const std::string_view str, const std::regex& regex, const std::string_view replacePattern,
                                      const std::regex_constants::match_flag_type flags = std::regex_constants::match_default);
} // namespace ig
