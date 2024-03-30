#pragma once
/**
 * 'Igniter' assumes that type of 'std::string' instances must be a 'UTF-8 encoded string'.
 * 'Igniter' assumes that source code files are encoded in UTF-8.
 * Also, All strings stored in memory are instances of std::string.
 * When string comes in either UTF-16 encoded or as a wide characters. It must converted to UTF-8 encoded string using
 * 'Narrower' function. Basically, 'Igniter' is targeting the 'Windows Platform'. Sometimes, It should pass string as
 * 'Wider Character String'. In such cases, you must use 'Wider' functions to convert UTF-8 encoded string to wider character string.
 */
#include <Igniter.h>
#include <Core/HashUtils.h>

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
        String(const String& other);
        String(const std::string_view strView);
        String(const std::wstring_view strView);
        ~String() = default;

        String& operator=(const String& rhs);
        String& operator=(const std::string_view rhs);
        String& operator=(const std::wstring_view rhs);

        [[nodiscard]] operator std::string() const { return ToStandard(); }
        [[nodiscard]] operator std::string_view() const { return ToStringView(); }
        [[nodiscard]] operator std::wstring() const { return ToWideString(); }

        [[nodiscard]] operator bool() const noexcept { return IsValid(); }

        [[nodiscard]] bool operator==(const String& rhs) const noexcept;
        [[nodiscard]] bool operator==(const std::string_view rhs) const noexcept;
        [[nodiscard]] bool operator==(const std::wstring_view rhs) const;

        void SetString(const std::string_view strView);

        [[nodiscard]] std::string ToStandard() const;
        [[nodiscard]] std::string_view ToStringView() const;
        [[nodiscard]] const char* ToCString() const;
        [[nodiscard]] std::wstring ToWideString() const;
        [[nodiscard]] fs::path ToPath() const;

        [[nodiscard]] static String FromPath(const fs::path& path);

        [[nodiscard]] uint64_t GetHash() const noexcept { return hashOfString; }
        [[nodiscard]] bool IsValid() const noexcept { return hashOfString != InvalidHashVal; }

        static std::vector<std::pair<uint64_t, std::string_view>> GetCachedStrings();

    private:
        [[nodiscard]] static constexpr uint64_t EvalHash(const std::string_view strView) noexcept;

        using HashStringMap = robin_hood::unordered_map<uint64_t, std::string>;
        [[nodiscard]] static HashStringMap& GetHashStringMap();
        [[nodiscard]] static SharedMutex& GetHashStringMapMutex();

    private:
        uint64_t hashOfString{ 0 };
    };

    inline String operator""_fs(const char* str, const size_t count)
    {
        return std::string_view{ str, count };
    }

    inline String operator""_fs(const wchar_t* wstr, const size_t count)
    {
        return String(std::wstring_view{ wstr, count });
    }
} // namespace ig

template <>
class std::hash<ig::String>
{
public:
    [[nodiscard]] size_t operator()(const ig::String& str) const noexcept { return str.GetHash(); }
};

template <>
struct std::formatter<ig::String>
{
public:
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FrameContext>
    auto format(const ig::String& str, FrameContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", str.ToStringView());
    }
};