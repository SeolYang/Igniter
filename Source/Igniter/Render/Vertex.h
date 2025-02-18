#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    /* [-1, 1] -> [0, 255] */
    inline U32 EncodeNormalX8Y8Z8(const Vector3& normal)
    {
        return (U32)((normal.x + 1.f) * 127.5f) |
            ((U32)((normal.y + 1.f) * 127.5f) << 8) |
            ((U32)((normal.z + 1.f) * 127.5f) << 16);
    }

    inline Vector3 DecodeNormalX8Y8Z8(const U32 encodedNormal)
    {
        constexpr float kInvFactor = 1.f / 127.5f;
        return Vector3{
            ((F32)(encodedNormal & 0xFF) * kInvFactor) - 1.f,
            ((F32)((encodedNormal >> 8) & 0xFF) * kInvFactor) - 1.f,
            ((F32)((encodedNormal >> 16) & 0xFF) * kInvFactor) - 1.f
        };
    }

    /* [-1, 1] -> [0, 1023] */
    inline U32 EncodeNormalX10Y10Z10(const Vector3& normal)
    {
        return (U32)((normal.x + 1.f) * 511.5f) |
            ((U32)((normal.y + 1.f) * 511.5f) << 10) |
            ((U32)((normal.z + 1.f) * 511.5f) << 20);
    }

    inline Vector3 DecodeNormalX10Y10Z10(const U32 encodedNormal)
    {
        constexpr float kInvFactor = 1.f / 511.5f;
        return Vector3{
            ((F32)(encodedNormal & 0x3FF) * kInvFactor) - 1.f,
            ((F32)((encodedNormal >> 10) & 0x3FF) * kInvFactor) - 1.f,
            ((F32)((encodedNormal >> 20) & 0x3FF) * kInvFactor) - 1.f
        };
    }

    /* RGBA32F -> RGBA8_UINT */
    inline U32 EncodeRGBA32F(const Vector4& rgba)
    {
        return (U32)(rgba.x * 255.f) |
            ((U32)(rgba.y * 255.f) << 8) |
            ((U32)(rgba.z * 255.f) << 16) |
            ((U32)(rgba.w * 255.f) << 24);
    }

    struct VertexBase
    {
        Vector3 Position;
        Vector3 Normal;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector2 TexCoords;
        U32 ColorRGBA8_U32;
    };

    struct Vertex
    {
        Vector3 Position;
        U32 QuantizedNormal;       /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        U32 QuantizedTangent;      /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        U32 QuantizedBitangent;    /* x: 10 Bits, y: 10 Bits, z: 10 Bits, Pad: 2 Bits, [-1, 1] -> [0, 1023] */
        U16 QuantizedTexCoords[2]; /* (F32, F32) -> (F16, F16) = HLSL(float16_t, float16_t); https://github.com/zeux/meshoptimizer/blob/master/src/quantization.cpp*/
        U32 ColorRGBA8_U32;        /* R8G8B8A8_UINT */
    };
} // namespace ig
