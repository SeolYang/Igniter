#pragma once
#include "Igniter/Asset/Common.h"

namespace ig
{
    struct AudioClipImportDesc
    {
        std::string_view Path;
    };

    struct AudioClipLoadDesc
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

    public:
        std::string Extension;
    };

    class Audio;

    class AudioClip
    {
    public:
        using ImportDesc = AudioClipImportDesc;
        using LoadDesc = AudioClipLoadDesc;
        using Desc = AssetDesc<AudioClip>;

    public:
        AudioClip(const Desc& snapshot, const Handle<Audio> newAudioHandle);
        AudioClip(const AudioClip&) = delete;
        AudioClip(AudioClip&& other) noexcept;
        ~AudioClip();

        AudioClip& operator=(const AudioClip&) = delete;
        AudioClip& operator=(AudioClip&& other) noexcept;

        [[nodiscard]] const Desc& GetSnapshot() const noexcept { return snapshot; }
        [[nodiscard]] Handle<Audio> GetAudio() const noexcept { return audioHandle; }

    private:
        void Destroy();

    private:
        Desc snapshot{};
        Handle<Audio> audioHandle;
    };
}
