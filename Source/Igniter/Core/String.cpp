#include <Igniter.h>
#include <Core/String.h>

namespace ig
{
    constexpr uint64_t String::EvalHash(const std::string_view strView) noexcept
    {
        return EvalCRC64(strView);
    }

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

    String::String(const String& other) : hashOfString(other.hashOfString)
    {
    }

    String::String(const std::string_view strView)
    {
        SetString(strView);
    }

    String::String(const std::wstring_view strView)
    {
        SetString(Narrower(strView));
    }

    String& String::operator=(const String& rhs)
    {
        hashOfString = rhs.hashOfString;
        return *this;
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

    bool String::operator==(const String& rhs) const noexcept
    {
        return hashOfString == rhs.hashOfString;
    }

    bool String::operator==(const std::string_view rhs) const noexcept
    {
        return this->hashOfString == String::EvalHash(rhs);
    }

    bool String::operator==(const std::wstring_view rhs) const
    {
        const std::string narrower{ Narrower(rhs) };
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
                hashOfString = String::EvalHash(strView);
                IG_CHECK(hashOfString != InvalidHashVal);

                HashStringMap& hashStringMap{ GetHashStringMap() };
                ReadWriteLock lock{ GetHashStringMapMutex() };
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

    std::string String::ToStandard() const
    {
        return std::string{ ToStringView() };
    }

    std::string_view String::ToStringView() const
    {
        if (hashOfString == InvalidHashVal)
        {
            return std::string_view{ "#INVALID_UTF8_STRING" };
        }

        if (hashOfString == 0)
        {
            return std::string_view{ "" };
        }

        ReadOnlyLock lock{ GetHashStringMapMutex() };
        const HashStringMap& hashStringMap{ GetHashStringMap() };
        IG_CHECK(hashStringMap.contains(hashOfString));
        return hashStringMap.at(hashOfString);
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
        return fs::path{ ToStringView() };
    }

    String String::FromPath(const fs::path& path)
    {
        return String{ path.c_str() };
    }

    std::vector<std::pair<uint64_t, std::string_view>> String::GetCachedStrings()
    {
        ReadOnlyLock lock{ GetHashStringMapMutex() };
        HashStringMap& hashStringMap{ GetHashStringMap() };

        std::vector<std::pair<uint64_t, std::string_view>> cachedStrs;
        cachedStrs.reserve(hashStringMap.size());
        for (const auto& itr : hashStringMap)
        {
            cachedStrs.emplace_back(itr.first, std::string_view{ itr.second });
        }
        cachedStrs.emplace_back(0, std::string_view{ "" });

        return cachedStrs;
    }
} // namespace ig
