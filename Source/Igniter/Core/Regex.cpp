#include <Igniter.h>
#include <Core/Regex.h>

namespace ig
{
    bool RegexMatch(const String str, const std::regex& regex, const std::regex_constants::match_flag_type flags /*= std::regex_constants::match_default*/)
    {
        if (!str.IsValid())
        {
            return false;
        }

        return std::regex_match(str.ToCString(), regex, flags);
    }

    std::vector<String> RegexMatchN(const String str, const std::regex& regex, const std::regex_constants::match_flag_type flags /*= std::regex_constants::match_default*/)
    {
        std::vector<String> result{};
        if (str.IsValid())
        {
            std::cmatch matches{};
            if (std::regex_match(str.ToCString(), matches, regex, flags))
            {
                result.reserve(matches.size());
                for (const auto& subMatch : matches)
                {
                    result.emplace_back(std::string_view{ subMatch.first, static_cast<size_t>(subMatch.length()) });
                }
            }
        }

        return result;
    }

    std::vector<String> RegexSearch(const String str, const std::regex& regex)
    {
        std::vector<String> result{};
        if (str.IsValid())
        {
            const char* searchBegin{ str.ToCString() };
            std::cmatch match{};
            while (std::regex_search(searchBegin, match, regex))
            {
                result.emplace_back(std::string_view{ match[0].first, static_cast<size_t>(match[0].length()) });
                searchBegin = match.suffix().first;
            }
        }

        return result;
    }

    String RegexReplace(const String str, const std::regex& regex, const String replacePattern, const std::regex_constants::match_flag_type flags /*= std::regex_constants::match_default*/)
    {
        if (!str.IsValid())
        {
            return {};
        }

        return String{ std::regex_replace(str.ToStandard(), regex, replacePattern.ToStandard(), flags) };
    }
} // namespace ig