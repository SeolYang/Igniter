#include "Igniter/Igniter.h"
#include "Igniter/Audio/AudioSystem.h"
#include "Igniter/Asset/AudioClipLoader.h"

namespace ig
{
    AudioClipLoader::AudioClipLoader(AudioSystem& audioSystem)
        : audioSystem(&audioSystem)
    {}

    Result<AudioClip, EAudioClipLoadError> AudioClipLoader::Load(const AudioClip::Desc& desc)
    {
        IG_CHECK(audioSystem != nullptr);
        const AssetInfo& assetInfo{desc.Info};
        //const AudioClip::LoadDesc& loadDesc{desc.LoadDescriptor};
        if (!assetInfo.IsValid())
        {
            return MakeFail<AudioClip, EAudioClipLoadError::InvalidAssetInfo>();
        }
        if (assetInfo.GetCategory() != EAssetCategory::Audio)
        {
            return MakeFail<AudioClip, EAudioClipLoadError::AssetCategoryMismatch>();
        }

        const Path assetPath = MakeAssetPath(EAssetCategory::Audio, assetInfo.GetGuid());
        const Handle<Audio> audioHandle{audioSystem->CreateAudio(assetPath.string())};
        if (!audioHandle)
        {
            return MakeFail<AudioClip, EAudioClipLoadError::FailedToAllocateHandle>();
        }

        return MakeSuccess<AudioClip, EAudioClipLoadError>(desc, audioHandle);
    }
}
