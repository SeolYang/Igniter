#pragma once
/**
 * 'Igniter' assumes that type of 'std::string' instances must be a 'UTF-8 encoded string'.
 * 'Igniter' assumes that source code files are encoded in UTF-8.
 * Also, All strings stored in memory are instances of std::string.
 * When string comes in either UTF-16 encoded or as a wide characters. It must converted to UTF-8 encoded string using
 * 'Narrower' function. Basically, 'Igniter' is targeting the 'Windows Platform'. Sometimes, It should pass string as
 * 'Wider Character String'. In such cases, you must use 'Wider' functions to convert UTF-8 encoded string to wider character string.
 */
#include <Core/Container.h>
#include <Core/Mutex.h>
#include <Core/HashUtils.h>
#include <string>
#ifndef UTF_CPP_CPLUSPLUS
    #define UTF_CPP_CPLUSPLUS 201703L
#endif
#include <utf8cpp/utf8.h>

namespace ig
{
    inline std::wstring Wider(const std::string_view from)
    {
        std::wstring result;
        utf8::utf8to16(from.begin(), from.end(), std::back_inserter(result));
        return result;
    }

    inline std::string Narrower(const std::wstring_view from)
    {
        std::string result;
        utf8::utf16to8(from.begin(), from.end(), std::back_inserter(result));
        return result;
    }

    inline bool IsValidUTF8(const std::string_view string)
    {
        return utf8::is_valid(string);
    }

    /**
     * The String class is light-weight strictly UTF-8 Encoded String.
     * It provides light-weight string-string comparing based on 'CRC64' hash.
     * And also, it is convertible as std::string.
     *
     * The String class will strictly check that the input string properly encoded as UTF-8.
     *
     * It treats empty string as 'invalid'.
     * It treats not properly encoded as UTF-8 string as 'invalid'.
     * It treats comparing with 'invalid' string instance is always 'false'.
     *
     * The AsString() function will return 'empty' string for the 'invalid' string instance.
     *
     * The construction of ig::String instances can lead to performance issues due to CRC64 hash evaluation.
     * Therefore, it is recommended to avoid creating them anew and using of SetString in loops.
     */
    class String final
    {
    public:
        String() = default;
        String(std::string_view name);
        String(const String& name);
        ~String() = default;

        void SetString(std::string_view string);

        bool IsValid() const { return hashOfString != InvalidHashVal; }
        uint64_t GetHash() const { return hashOfString; }

        operator std::string() const { return AsString(); }
        operator std::string_view() const { return AsStringView(); }
        std::string AsString() const;
        std::string_view AsStringView() const;
        const char* AsCStr() const;
        std::wstring AsWideString() const;

        operator bool() const { return IsValid(); }

        String& operator=(const String& rhs);
        String& operator=(std::string_view rhs);
        String& operator=(const std::string& rhs);

        bool operator==(std::string_view rhs) const;
        bool operator!=(const std::string_view rhs) const { return !(*this == rhs); }
        bool operator==(const String& rhs) const;
        bool operator!=(const String& rhs) const { return !(*this == rhs); }

        static std::vector<std::pair<uint64_t, std::string_view>> GetCachedStrings();

    private:
        using HashStringMap = robin_hood::unordered_map<uint64_t, std::string>;
        static HashStringMap hashStringMap;
        static SharedMutex hashStringMapMutex;

    private:
        uint64_t hashOfString = InvalidHashVal;
    };

    inline String operator""_fs(const char* str, const size_t count)
    {
        return String(std::string_view{ str, count });
    }

    inline String operator""_fs(const wchar_t* wstr, const size_t count)
    {
        return String(Narrower(std::wstring_view{wstr, count}));
    }
} // namespace ig

namespace std
{
    template <>
    class hash<ig::String>
    {
    public:
        size_t operator()(const ig::String& name) const { return name.GetHash(); }
    };
} // namespace std
