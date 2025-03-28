#pragma once
#include "Igniter/Core/Result.h"
#include "Igniter/Asset/AudioClip.h"

namespace ig
{
    class AssetManager;

    enum class EAudioClipImportError : U8
    {
        Success,
        FileDoesNotExist,
        FailedToSaveMetadata,
        FailedToCopyAudioFile,
    };

    class AudioClipImporter
    {
    public:
        AudioClipImporter() = default;
        AudioClipImporter(const AudioClipImporter&) = delete;
        AudioClipImporter(AudioClipImporter&&) noexcept = delete;
        ~AudioClipImporter() = default;

        AudioClipImporter& operator=(const AudioClipImporter&) = delete;
        AudioClipImporter& operator=(AudioClipImporter&&) noexcept = delete;

        Result<AudioClip::Desc, EAudioClipImportError> Import(const AudioClip::ImportDesc& desc);
    };
}
