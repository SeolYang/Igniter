#pragma once
#include <Core/Container.h>
#include <Core/Handle.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
#include <Core/String.h>
#include <Filesystem/Utils.h>

#ifndef IG_TEXT
    #define IG_TEXT(x) #x
#endif

namespace ig
{
    namespace fs = std::filesystem;

    enum class EAssetType
    {
        Unknown,
        Texture,
        StaticMesh,
        SkeletalMesh,
        Shader,
        Audio,
        Script,
        // Scene
    };

    /* Refer to {ResourcePath}.metadata */
    fs::path MakeResourceMetadataPath(fs::path resPath);

    /* Refer to ./Asset/{AssetType}/{GUID} */
    fs::path MakeAssetPath(const EAssetType type, const xg::Guid& guid);

    /* Refer to ./Assets/{AssetType}/{GUID}.metadata */
    fs::path MakeAssetMetadataPath(const EAssetType type, const xg::Guid& guid);

    struct ResourceInfo
    {
    public:
        json& Serialize(json& archive) const
        {
            const ResourceInfo& info = *this;
            IG_CHECK(info.Type != EAssetType::Unknown);
            IG_SERIALIZE_ENUM_JSON(ResourceInfo, archive, info, Type);
            return archive;
        }

        const json& Deserialize(const json& archive)
        {
            ResourceInfo& info = *this;
            IG_DESERIALIZE_ENUM_JSON(AssetMetadata, archive, info, Type, EAssetType::Unknown);

            IG_CHECK(info.Type != EAssetType::Unknown);
            return archive;
        }

    public:
        EAssetType Type = EAssetType::Unknown;
    };

    /* Common Asset Metadata */
    struct AssetInfo
    {
    public:
        json& Serialize(json& archive) const
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

        const json& Deserialize(const json& archive)
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

    public:
        uint64_t CreationTime = 0;
        xg::Guid Guid{};
        std::string SrcResPath{};
        EAssetType Type = EAssetType::Unknown;
        bool bIsPersistent = false;
    };

    inline bool HasImportedBefore(const fs::path& resPath)
    {
        return fs::exists(MakeResourceMetadataPath(resPath));
    }

    namespace details
    {
        constexpr inline std::string_view MetadataExt = ".metadata";
        constexpr inline std::string_view ResourceRootPath = "Resources";
        constexpr inline std::string_view AssetRootPath = "Assets";
        constexpr inline std::string_view TextureAssetRootPath = "Assets\\Textures";
        constexpr inline std::string_view StaticMeshAssetRootPath = "Assets\\StaticMeshes";
        constexpr inline std::string_view SkeletalMeshAssetRootPath = "Assets\\SkeletalMeshes";
        constexpr inline std::string_view ShaderAssetRootPath = "Assets\\Shaders";
        constexpr inline std::string_view AudioAssetRootPath = "Assets\\Audios";
        constexpr inline std::string_view ScriptAssetRootPath = "Assets\\Scripts";

        inline void CreateAssetDirectories()
        {
            if (!fs::exists(TextureAssetRootPath))
            {
                fs::create_directories(TextureAssetRootPath);
            }

            if (!fs::exists(StaticMeshAssetRootPath))
            {
                fs::create_directories(StaticMeshAssetRootPath);
            }

            if (!fs::exists(SkeletalMeshAssetRootPath))
            {
                fs::create_directories(SkeletalMeshAssetRootPath);
            }

            if (!fs::exists(ShaderAssetRootPath))
            {
                fs::create_directories(ShaderAssetRootPath);
            }

            if (!fs::exists(AudioAssetRootPath))
            {
                fs::create_directories(AudioAssetRootPath);
            }

            if (!fs::exists(ScriptAssetRootPath))
            {
                fs::create_directories(ScriptAssetRootPath);
            }
        }
    } // namespace details
} // namespace ig
