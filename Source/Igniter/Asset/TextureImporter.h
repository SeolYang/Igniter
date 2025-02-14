#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Asset/Texture.h"

struct ID3D11Device;

namespace ig
{
    enum class ETextureImportStatus
    {
        Success,
        FileDoesNotExists,
        UnsupportedExtension,
        FailedLoadFromFile,
        InvalidDimensions,
        InvalidVolumemap,
        UnknownFormat,
        FailedGenerateMips,
        FailedCompression,
        FailedSaveMetadataToFile,
        FailedSaveAssetToFile
    };

    class AssetManager;

    class TextureImporter final
    {
        friend class AssetManager;

    public:
        TextureImporter();
        TextureImporter(const TextureImporter&) = delete;
        TextureImporter(TextureImporter&&) noexcept = delete;
        ~TextureImporter();

        TextureImporter& operator=(const TextureImporter&) = delete;
        TextureImporter& operator=(TextureImporter&&) noexcept = delete;

    private:
        Result<Texture::Desc, ETextureImportStatus> Import(const String resPathStr, TextureImportDesc config);

    private:
        Mutex compressionMutex{};
        ID3D11Device* d3d11Device{nullptr};
    };
} // namespace ig
