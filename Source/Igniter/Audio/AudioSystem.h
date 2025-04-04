#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/HandleStorage.h"

namespace ig
{
    // Asset: Audio
    // In-Engine Representation: Audio Clip
    class Engine;

    /* Mimickinng FMOD::Sound */
    class Audio;
    struct AudioSourceComponent;

    class AudioSystem
    {
    public:
        AudioSystem();
        AudioSystem(const AudioSystem&) = delete;
        AudioSystem(AudioSystem&&) noexcept = delete;
        ~AudioSystem();

        AudioSystem& operator=(const AudioSystem&) = delete;
        AudioSystem& operator=(AudioSystem&&) noexcept = delete;

        Handle<Audio> CreateAudio(const std::string_view path);
        void Destroy(const Handle<Audio> audioHandle);

        void Update(Registry& registry, const float deltaTime);

    private:
        FMOD::Channel* UpdateAudioStatusUnsafe(const Entity entity, AudioSourceComponent& audioSource);
        FMOD::Sound* LookupUnsafe(const Handle<Audio> audio);

    private:
        constexpr static int kMaxChannels = 512;

    private:
        FMOD::System* system = nullptr;

        UnorderedMap<Entity, FMOD::Channel*> channelMap;

        mutable SharedMutex audioClipStorageMutex;
        HandleStorage<FMOD::Sound*> audioClipStorage;
    };
}
