#include <PCH.h>
#include <Core/String.h>
#include <Core/HashUtils.h>

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
        if (!stringView.empty() && IsValidUTF8(stringView))
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

    std::string String::AsString() const
    {
        return std::string{ AsStringView() };
    }

    std::string_view String::AsStringView() const
    {
        if (hashOfString == InvalidHashVal)
        {
            return std::string_view{};
        }

        ReadOnlyLock lock{ hashStringMapMutex };
        return hashStringMap[hashOfString];
    }

    const char* String::AsCStr() const
    {
        return AsStringView().data();
    }

    std::wstring String::AsWideString() const
    {
        return Wider(AsStringView());
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
