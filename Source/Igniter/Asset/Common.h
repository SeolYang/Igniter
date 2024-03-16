#pragma once
#include <Core/Container.h>
#include <Core/Handle.h>
#include <Core/String.h>
#include <Core/Log.h>
#include <Core/Serializable.h>
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

    /* #sy_todo Resource Metadata, Asset Metadata 얘네를 추상화 시키기(Version) */
    /* Common Resource Metadata */
    template <uint64_t DerivedVersion, uint64_t BaseVersion = 3>
    struct ResourceMetadata : public Serializable<DerivedVersion + BaseVersion>
    {
    public:
        virtual json& Serialize(json& archive) const
        {
            Serializable<DerivedVersion + BaseVersion>::Serialize(archive);

            const ResourceMetadata& metadata = *this;
            IG_CHECK(metadata.AssetType != EAssetType::Unknown);

            IG_SERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType);
            IG_SERIALIZE_JSON(ResourceMetadata, archive, metadata, bIsPersistent);

            return archive;
        }

        virtual const json& Deserialize(const json& archive)
        {
            Serializable<DerivedVersion + BaseVersion>::Deserialize(archive);

            ResourceMetadata& metadata = *this;
            IG_DESERIALIZE_ENUM_JSON(ResourceMetadata, archive, metadata, AssetType, EAssetType::Unknown);
            IG_DESERIALIZE_JSON(ResourceMetadata, archive, metadata, bIsPersistent);

            IG_CHECK(metadata.AssetType != EAssetType::Unknown);
            return archive;
        }

    public:
        using CommonMetadata = ResourceMetadata;

        EAssetType AssetType = EAssetType::Unknown;
        bool bIsPersistent = false;
    };

    /* Common Asset Metadata */
    template <uint64_t DerivedVersion, uint64_t BaseVersion = 3>
    struct AssetMetadata : public Serializable<DerivedVersion + BaseVersion>
    {
    public:
        virtual json& Serialize(json& archive) const
        {
            Serializable<DerivedVersion + BaseVersion>::Serialize(archive);

            const AssetMetadata& metadata = *this;
            IG_CHECK(metadata.Guid.isValid());
            IG_CHECK(!metadata.SrcResPath.empty());
            IG_CHECK(metadata.Type != EAssetType::Unknown);

            IG_SERIALIZE_JSON(AssetMetadata, archive, metadata, CreationTime);
            IG_SERIALIZE_GUID_JSON(AssetMetadata, archive, metadata, Guid);
            IG_SERIALIZE_JSON(AssetMetadata, archive, metadata, SrcResPath);
            IG_SERIALIZE_ENUM_JSON(AssetMetadata, archive, metadata, Type);
            IG_SERIALIZE_JSON(AssetMetadata, archive, metadata, bIsPersistent);

            return archive;
        }

        virtual const json& Deserialize(const json& archive)
        {
            Serializable<DerivedVersion + BaseVersion>::Deserialize(archive);

            AssetMetadata& metadata = *this;
            IG_DESERIALIZE_JSON(AssetMetadata, archive, metadata, CreationTime);
            IG_DESERIALIZE_GUID_JSON(AssetMetadata, archive, metadata, Guid);
            IG_DESERIALIZE_JSON(AssetMetadata, archive, metadata, SrcResPath);
            IG_DESERIALIZE_ENUM_JSON(AssetMetadata, archive, metadata, Type, EAssetType::Unknown);
            IG_DESERIALIZE_JSON(AssetMetadata, archive, metadata, bIsPersistent);

            IG_CHECK(metadata.Guid.isValid());
            IG_CHECK(!metadata.SrcResPath.empty());
            IG_CHECK(metadata.Type != EAssetType::Unknown);
            return archive;
        }

    public:
        using CommonMetadata = AssetMetadata;

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

        template <typename T, typename LogCategory>
        void LogVersionMismatch(const T& instance, const std::string_view infoMessage = "")
        {
            IG_LOG(LogCategory, IG_TEXT(T) " version does not match.\n\tFound: {}\nExpected: {}\nInfos: {}",
                   instance.Version, T::LatestVersion, infoMessage);
        }

        template <typename T, typename MismatchLogCategory = ig::LogWarn>
        std::optional<T> LoadMetadataFromFile(const fs::path& resMetaPath)
        {
            if (!fs::exists(resMetaPath))
            {
                return std::nullopt;
            }

            std::optional<T> metadata = LoadSerializedDataFromJsonFile<T>(resMetaPath);
            if (!metadata)
            {
                return std::nullopt;
            }

            if (metadata->IsLatestVersion())
            {
                details::LogVersionMismatch<T, MismatchLogCategory>(*metadata, resMetaPath.string());
            }

            return metadata;
        }

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
