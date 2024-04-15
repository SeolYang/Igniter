#include <Igniter.h>
#include <Core/String.h>
#include <Core/Regex.h>

namespace ig
{
    String::HashStringMap& String::GetHashStringMap()
    {
        static HashStringMap hashStringMap{};
        return hashStringMap;
    }

    SharedMutex& String::GetHashStringMapMutex()
    {
        static SharedMutex hashStringMapMutex{};
        return hashStringMapMutex;
    }

    String::String(const std::string_view strView)
    {
        SetString(strView);
    }

    String::String(const std::wstring_view strView)
    {
        SetString(Narrower(strView));
    }

    String& String::operator=(const std::string_view rhs)
    {
        SetString(rhs);
        return *this;
    }

    String& String::operator=(const std::wstring_view rhs)
    {
        SetString(Narrower(rhs));
        return *this;
    }

    bool String::operator==(const std::string_view rhs) const noexcept
    {
        return this->hashOfString == XXH64(rhs.data(), rhs.length(), 0);
    }

    bool String::operator==(const std::wstring_view rhs) const
    {
        const std::string narrower{Narrower(rhs)};
        return *this == narrower;
    }

    void String::SetString(const std::string_view strView)
    {
        if (utf8::is_valid(strView))
        {
            if (strView.empty())
            {
                hashOfString = 0;
            }
            else
            {
                hashOfString = XXH64(strView.data(), strView.length(), 0);
                IG_CHECK(hashOfString != InvalidHashVal);

                HashStringMap& hashStringMap{GetHashStringMap()};
                ReadWriteLock  lock{GetHashStringMapMutex()};
                if (!hashStringMap.contains(hashOfString))
                {
                    hashStringMap[hashOfString] = strView;
                }
            }
        }
        else
        {
            hashOfString = InvalidHashVal;
            IG_CHECK_NO_ENTRY();
        }
    }

    const std::string& String::ToStandard() const
    {
        static const std::string InvalidUtf8String{"#INVALID_UTF8_STR"};
        static const std::string Empty{};
        if (hashOfString == InvalidHashVal)
        {
            return InvalidUtf8String;
        }

        if (hashOfString == 0)
        {
            return Empty;
        }

        ReadOnlyLock         lock{GetHashStringMapMutex()};
        const HashStringMap& hashStringMap{GetHashStringMap()};
        IG_CHECK(hashStringMap.contains(hashOfString));
        return hashStringMap.at(hashOfString);
    }

    std::string_view String::ToStringView() const
    {
        return std::string_view{ToStandard()};
    }

    const char* String::ToCString() const
    {
        return ToStringView().data();
    }

    std::wstring String::ToWideString() const
    {
        return Wider(ToStringView());
    }

    fs::path String::ToPath() const
    {
        return fs::path{ToStringView()};
    }

    String String::FromPath(const fs::path& path)
    {
        return String{path.c_str()};
    }

    std::vector<String> String::Split(const String delimiter) const
    {
        auto splitStrViews{
            ToStringView() |
            views::split(delimiter.ToStringView()) |
            views::transform([](auto&& element)
            {
                return String{std::string_view{&*element.begin(), static_cast<size_t>(ranges::distance(element))}};
            })
        };

        return std::vector<String>{splitStrViews.begin(), splitStrViews.end()};
    }

    std::vector<std::pair<uint64_t, std::string_view>> String::GetCachedStrings()
    {
        ReadOnlyLock   lock{GetHashStringMapMutex()};
        HashStringMap& hashStringMap{GetHashStringMap()};

        std::vector<std::pair<uint64_t, std::string_view>> cachedStrs;
        cachedStrs.reserve(hashStringMap.size());
        for (const auto& itr : hashStringMap)
        {
            cachedStrs.emplace_back(itr.first, std::string_view{itr.second});
        }
        cachedStrs.emplace_back(0, std::string_view{""});

        return cachedStrs;
    }

    String CamelCaseToTitleCase(const String& str)
    {
        if (!str.IsValid())
        {
            return String{};
        }

        static const std::regex LetterSpacingRegex{"([a-z])([A-Z])", std::regex_constants::optimize};
        static const std::regex NumberSpacingRegex{"([a-zA-Z])([0-9])", std::regex_constants::optimize};
        static const String ReplacePattern{"$1 $2"};

        String spacedStr = RegexReplace(str, LetterSpacingRegex, ReplacePattern);
        spacedStr = RegexReplace(spacedStr, NumberSpacingRegex, ReplacePattern);
        std::string  value{spacedStr.ToStandard()};
        std::transform(value.begin(), value.begin() + 1, value.begin(), [](const char character)
        {
            return (char)std::toupper((int)character);
        });

        return String{value};
    }
} // namespace ig
