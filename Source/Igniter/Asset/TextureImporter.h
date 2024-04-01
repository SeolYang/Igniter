#pragma once
#include <Igniter.h>
#include <Core/Result.h>
#include <Asset/Texture.h>

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

    class TextureImporter final
    {
    public:
        TextureImporter();
        TextureImporter(const TextureImporter&) = delete;
        TextureImporter(TextureImporter&&) noexcept = delete;
        ~TextureImporter();

        TextureImporter& operator=(const TextureImporter&) = delete;
        TextureImporter& operator=(TextureImporter&&) noexcept = delete;

        Result<Texture::Desc, ETextureImportStatus> Import(const String resPathStr, TextureImportDesc config);

    private:
        Mutex compressionMutex{};
        ID3D11Device* d3d11Device{ nullptr };
    };
} // namespace ig