#include "Igniter/Igniter.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/Core/Json.h"
#include "Igniter/Audio/AudioSystem.h"
#include "Igniter/Asset/AudioClip.h"

namespace ig
{
    Json& AudioClipLoadDesc::Serialize(Json& archive) const
    {
        IG_SERIALIZE_TO_JSON(AudioClipLoadDesc, archive, Extension);
        return archive;
    }

    const Json& AudioClipLoadDesc::Deserialize(const Json& archive)
    {
        IG_DESERIALIZE_FROM_JSON(AudioClipLoadDesc, archive, Extension);
        return archive;
    }

    AudioClip::AudioClip(const Desc& snapshot, const Handle<Audio> newAudioHandle)
        : snapshot(snapshot)
        , audioHandle(newAudioHandle)
    {}

    AudioClip::AudioClip(AudioClip&& other) noexcept
        : snapshot(other.snapshot)
        , audioHandle(std::exchange(other.audioHandle, {}))
    {}

    AudioClip::~AudioClip()
    {
        Destroy();
    }

    AudioClip& AudioClip::operator=(AudioClip&& other) noexcept
    {
        Destroy();
        snapshot = other.snapshot;
        audioHandle = std::exchange(other.audioHandle, {});
        return *this;
    }

    void AudioClip::Destroy()
    {
        if (audioHandle)
        {
            Engine::GetAudioSystem().Destroy(audioHandle);
            audioHandle = {};
        }
    }
}
