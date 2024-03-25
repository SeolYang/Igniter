#include <Igniter.h>
#include <Core/String.h>

namespace ig
{
    String::HashStringMap String::hashStringMap = {};
    SharedMutex String::hashStringMapMutex;

    String::String(const std::string_view stringView)
        : hashOfString(InvalidHashVal)
    {
        SetString(stringView);
    }

    String::String(const String& other)
        : hashOfString(other.hashOfString) {}

    void String::SetString(const std::string_view stringView)
    {
        if (!stringView.empty() && utf8::is_valid(stringView))
        {
            hashOfString = EvalCRC64(stringView);

            ReadWriteLock lock{ hashStringMapMutex };
            if (!hashStringMap.contains(hashOfString))
            {
                hashStringMap[hashOfString] = stringView;
            }
        }
        else
        {
            hashOfString = InvalidHashVal;
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
            return std::string_view{};
        }

        ReadOnlyLock lock{ hashStringMapMutex };
        IG_CHECK(hashStringMap.contains(hashOfString));
        return hashStringMap[hashOfString];
    }

    const char* String::ToCString() const
    {
        return ToStringView().data();
    }

    std::wstring String::ToWideString() const
    {
        return Wider(ToStringView());
    }

    String& String::operator=(const std::string& rhs)
    {
        SetString(rhs);
        return *this;
    }

    String& String::operator=(const std::string_view rhs)
    {
        SetString(rhs);
        return *this;
    }

    String& String::operator=(const String& rhs)
    {
        hashOfString = rhs.hashOfString;
        return *this;
    }

    bool String::operator==(const std::string_view rhs) const
    {
        return (*this) == String{ rhs };
    }

    bool String::operator==(const String& rhs) const
    {
        return (IsValid() && rhs.IsValid()) ? hashOfString == rhs.hashOfString : false;
    }

    std::vector<std::pair<uint64_t, std::string_view>> String::GetCachedStrings()
    {
        ReadOnlyLock lock{ hashStringMapMutex };
        std::vector<std::pair<uint64_t, std::string_view>> cachedStrs;
        cachedStrs.reserve(hashStringMap.size());

        for (const auto& itr : hashStringMap)
        {
            cachedStrs.emplace_back(itr.first, std::string_view{ itr.second });
        }

        return cachedStrs;
    }
} // namespace ig
