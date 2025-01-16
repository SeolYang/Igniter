#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/Regex.h"

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

    U64 String::CalculateHash(const std::string_view str)
    {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
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
        return this->hashOfString == CalculateHash(rhs);
    }

    bool String::operator==(const std::wstring_view rhs) const
    {
        const std::string narrower{Narrower(rhs)};
        return *this == narrower;
    }

    /* #todo string이 rvalue로 넘어올 때, 아예 copy 없이 hash 맵 안에 넣을 수 있도록 수정 할 것 */
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
                hashOfString = CalculateHash(strView);
                IG_CHECK(hashOfString != InvalidHashVal);

                HashStringMap& hashStringMap{GetHashStringMap()};
                ReadWriteLock lock{GetHashStringMapMutex()};
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

        ReadOnlyLock lock{GetHashStringMapMutex()};
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

    Path String::ToPath() const
    {
        return Path{ToStringView()};
    }

    String String::FromPath(const Path& path)
    {
        return String{path.c_str()};
    }

    Vector<String> String::Split(const String delimiter) const
    {
        auto splitStrViews{
            ToStringView() | views::split(delimiter.ToStringView()) |
            views::transform(
                [](auto&& element)
                {
                    return String{std::string_view{&*element.begin(), static_cast<Size>(ranges::distance(element))}};
                })};

        // results의 크기를 미리 reserve 할 수 있는 방법이 없을까?
        Vector<String> results{};
        for (const String result : splitStrViews)
        {
            results.emplace_back(result);
        }

        return results;
    }

    String String::ToTitleCase() const
    {
        if (!IsValid())
        {
            return String{};
        }

        static const std::regex LetterSpacingRegex{"([a-z])([A-Z])", std::regex_constants::optimize};
        static const std::regex NumberSpacingRegex{"([a-zA-Z])([0-9])", std::regex_constants::optimize};
        static const String ReplacePattern{"$1 $2"};

        String spacedStr = RegexReplace(*this, LetterSpacingRegex, ReplacePattern);
        spacedStr = RegexReplace(spacedStr, NumberSpacingRegex, ReplacePattern);
        std::string value{spacedStr.ToStandard()};
        std::transform(value.begin(), value.begin() + 1, value.begin(), [](const char character)
                       { return (char)std::toupper((int)character); });
        return String{value};
    }

    int String::Compare(const String other) const noexcept
    {
        if (this->hashOfString == other.hashOfString)
        {
            return 0;
        }

        return this->ToStringView().compare(other.ToStringView());
    }

    Vector<std::pair<uint64_t, std::string_view>> String::GetCachedStrings()
    {
        ReadOnlyLock lock{GetHashStringMapMutex()};
        HashStringMap& hashStringMap{GetHashStringMap()};

        Vector<std::pair<uint64_t, std::string_view>> cachedStrs;
        cachedStrs.reserve(hashStringMap.size());
        for (const auto& itr : hashStringMap)
        {
            cachedStrs.emplace_back(itr.first, std::string_view{itr.second});
        }
        cachedStrs.emplace_back(0, std::string_view{""});

        return cachedStrs;
    }
} // namespace ig
