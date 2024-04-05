#pragma once
#include <Igniter.h>

namespace ig::details
{
    /* #sy_ref https://github.com/graeme-hill/crossguid */
    constexpr uint8_t HexDigitToByte(char hexChar)
    {
        // 0-9
        if (hexChar > 47 && hexChar < 58)
            return hexChar - 48;

        // a-f
        if (hexChar > 96 && hexChar < 103)
            return hexChar - 87;

        // A-F
        if (hexChar > 64 && hexChar < 71)
            return hexChar - 55;

        return 0;
    }

    constexpr bool IsValidHexChar(const char hexChar)
    {
        // 0-9
        if (hexChar > 47 && hexChar < 58)
            return true;

        // a-f
        if (hexChar > 96 && hexChar < 103)
            return true;

        // A-F
        if (hexChar > 64 && hexChar < 71)
            return true;

        return false;
    }

    constexpr uint8_t TwoHexDigitToByte(const char frontHexChar, const char rearHexChar)
    {
        if (!IsValidHexChar(frontHexChar) || !IsValidHexChar(rearHexChar))
        {
            return 0;
        }

        return HexDigitToByte(frontHexChar) << 4 | HexDigitToByte(rearHexChar);
    }
}

namespace ig
{
    inline constexpr size_t GuidBytesLength = 16;
    using GuidBytes = std::array<uint8_t, GuidBytesLength>;

    constexpr static std::array<uint8_t, GuidBytesLength> GuidBytesFrom(const std::string_view strView)
    {
        std::array<uint8_t, GuidBytesLength> bytes{};
        char frontHexChar = '\0';
        char rearHexChar = '\0';
        bool bLookingForFrontChar = true;
        uint16_t nextByte = 0;

        for (const char& ch : strView)
        {
            if (ch == '-')
            {
                continue;
            }

            if (nextByte >= 16 || !details::IsValidHexChar(ch))
            {
                std::fill_n(bytes.begin(), bytes.size(), 0Ui8);
                break;
            }

            if (bLookingForFrontChar)
            {
                frontHexChar = ch;
                bLookingForFrontChar = false;
            }
            else
            {
                rearHexChar = ch;
                bytes[nextByte] = details::TwoHexDigitToByte(frontHexChar, rearHexChar);
                bLookingForFrontChar = true;
                ++nextByte;
            }
        }

        if (nextByte < 16)
        {
            std::fill_n(bytes.begin(), bytes.size(), 0Ui8);
        }

        return bytes;
    }
} // namespace ig