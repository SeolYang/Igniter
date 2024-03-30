#pragma once
#include <Core/Handle.h>
#include <Core/Result.h>
#include <Asset/Common.h>

namespace ig
{
    struct Material;
    template<>
    constexpr inline EAssetType AssetTypeOf_v<Material> = EAssetType::Material;

    struct Texture;
    struct Material
    {
    public:


    public:
        RefHandle<Texture> Diffuse;

    };
} // namespace ig