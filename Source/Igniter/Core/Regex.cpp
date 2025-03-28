#include "Igniter/Igniter.h"
#include "Igniter/Core/Regex.h"

namespace ig
{
    bool RegexMatch(
        const std::string_view str, const std::regex& regex, const std::regex_constants::match_flag_type flags /*= std::regex_constants::match_default*/)
    {
        if (str.empty())
        {
            return false;
        }

        return std::regex_match(str.data(), regex, flags);
    }

    Vector<std::string> RegexMatchN(const std::string_view str, const std::regex& regex, const std::regex_constants::match_flag_type flags
                                    /*= std::regex_constants::match_default*/)
    {
        Vector<std::string> result{};
        std::cmatch matches{};
        if (std::regex_match(str.data(), matches, regex, flags))
        {
            result.reserve(matches.size());
            for (const auto& subMatch : matches)
            {
                result.emplace_back(std::string_view{subMatch.first, static_cast<Size>(subMatch.length())});
            }
        }

        return result;
    }

    Vector<std::string> RegexSearch(const std::string_view str, const std::regex& regex)
    {
        Vector<std::string> result{};

        if (str.empty())
        {
            return result;
        }
        const char* searchBegin{str.data()};
        std::cmatch match{};
        while (std::regex_search(searchBegin, match, regex))
        {
            result.emplace_back(std::string_view{match[0].first, static_cast<Size>(match[0].length())});
            searchBegin = match.suffix().first;
        }

        return result;
    }

    std::string RegexReplace(const std::string_view str, const std::regex& regex, const std::string_view replacePattern,
                             const std::regex_constants::match_flag_type flags /*= std::regex_constants::match_default*/)
    {
        if (str.empty() || replacePattern.empty())
        {
            return std::string{};
        }
        
        return std::regex_replace(str.data(), regex, replacePattern.data(), flags);
    }
} // namespace ig
