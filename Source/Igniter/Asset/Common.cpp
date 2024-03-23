#include <PCH.h>
#include <Asset/Common.h>

namespace ig
{
    json& ResourceInfo::Serialize(json& archive) const
    {
        const ResourceInfo& info = *this;
        IG_CHECK(info.Type != EAssetType::Unknown);
        IG_SERIALIZE_ENUM_JSON(ResourceInfo, archive, info, Type);
        return archive;
    }

    const json& ResourceInfo::Deserialize(const json& archive)
    {
        ResourceInfo& info = *this;
        IG_DESERIALIZE_ENUM_JSON(ResourceInfo, archive, info, Type, EAssetType::Unknown);
        IG_CHECK(info.Type != EAssetType::Unknown);
        return archive;
    }

    json& AssetInfo::Serialize(json& archive) const
    {
        const AssetInfo& info = *this;
        IG_CHECK(info.Guid.isValid());
        IG_CHECK(!info.SrcResPath.empty());
        IG_CHECK(info.Type != EAssetType::Unknown);

        IG_SERIALIZE_JSON(AssetInfo, archive, info, CreationTime);
        IG_SERIALIZE_GUID_JSON(AssetInfo, archive, info, Guid);
        IG_SERIALIZE_JSON(AssetInfo, archive, info, SrcResPath);
        IG_SERIALIZE_ENUM_JSON(AssetInfo, archive, info, Type);
        IG_SERIALIZE_JSON(AssetInfo, archive, info, bIsPersistent);

        return archive;
    }

    const json& AssetInfo::Deserialize(const json& archive)
    {
        AssetInfo& info = *this;
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, CreationTime);
        IG_DESERIALIZE_GUID_JSON(AssetInfo, archive, info, Guid);
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, SrcResPath);
        IG_DESERIALIZE_ENUM_JSON(AssetInfo, archive, info, Type, EAssetType::Unknown);
        IG_DESERIALIZE_JSON(AssetInfo, archive, info, bIsPersistent);

        IG_CHECK(info.Guid.isValid());
        IG_CHECK(!info.SrcResPath.empty());
        IG_CHECK(info.Type != EAssetType::Unknown);
        return archive;
    }
} // namespace ig