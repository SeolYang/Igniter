#pragma once
#include "Igniter/Core/Result.h"
#include "Igniter/Asset/AudioClip.h"

namespace ig
{
    enum class EAudioClipLoadError : U8
    {
        Success,
        InvalidAssetInfo,
        AssetCategoryMismatch,
        AssetFileDoesNotExists,
        FailedToAllocateHandle,
    };

    class AudioSystem;

    class AudioClipLoader
    {
    public:
        explicit AudioClipLoader(AudioSystem& audioSystem);
        AudioClipLoader(const AudioClipLoader&) = delete;
        AudioClipLoader(AudioClipLoader&&) noexcept = delete;
        ~AudioClipLoader() = default;

        AudioClipLoader& operator=(const AudioClipLoader&) = delete;
        AudioClipLoader& operator=(AudioClipLoader&&) noexcept = delete;

        Result<AudioClip, EAudioClipLoadError> Load(const AudioClip::Desc& desc);

    private:
        AudioSystem* audioSystem = nullptr;
    };
}
