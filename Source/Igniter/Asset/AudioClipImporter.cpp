#include "Igniter/Igniter.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Filesystem/Utils.h"
#include "Igniter/Asset/Common.h"
#include "Igniter/Asset/AudioClipImporter.h"


namespace ig
{
    Result<AudioClip::Desc, EAudioClipImportError> AudioClipImporter::Import(const AudioClip::ImportDesc& desc)
    {
        const Path resPath{desc.Path};
        if (!fs::exists(resPath))
        {
            return MakeFail<AudioClip::Desc, EAudioClipImportError::FileDoesNotExist>();
        }

        const Path fileExtension = resPath.extension();
        const AssetInfo newAssetInfo{MakeVirtualPathPreferred(resPath.filename().replace_extension("").string()), EAssetCategory::Audio};

        const AudioClipLoadDesc newLoadDesc{
            .Extension = fileExtension.string(),
        };

        const Path newMetadataPath = MakeAssetMetadataPath(EAssetCategory::Audio, newAssetInfo.GetGuid());
        Json newAssetMetadata{};
        newAssetMetadata << newAssetInfo << newLoadDesc;
        if (!SaveJsonToFile(newMetadataPath, newAssetMetadata))
        {
            return MakeFail<AudioClip::Desc, EAudioClipImportError::FailedToSaveMetadata>();
        }
        const Path newAssetPath = MakeAssetPath(EAssetCategory::Audio, newAssetInfo.GetGuid());
        if (!fs::copy_file(resPath, newAssetPath))
        {
            return MakeFail<AudioClip::Desc, EAudioClipImportError::FailedToCopyAudioFile>();
        }

        IG_CHECK(newAssetInfo.IsValid());
        return MakeSuccess<AudioClip::Desc, EAudioClipImportError>(newAssetInfo, newLoadDesc);
    }
}
