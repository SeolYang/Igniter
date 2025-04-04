#include "Igniter/Igniter.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Audio/AudioSourceComponent.h"
#include "Igniter/Audio/AudioListenerComponent.h"
#include "Igniter/Audio/AudioSystem.h"

#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Core/Engine.h"

IG_DECLARE_PRIVATE_LOG_CATEGORY(AudioSystemLog);

namespace ig
{
    AudioSystem::AudioSystem()
    {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Fatal, "Failed to create FMOD system: {}", FMOD_ErrorString(result));
        }

        result = system->init(kMaxChannels, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Fatal, "Failed to initialize FMOD system: {}", FMOD_ErrorString(result));
        }

        // @todo CVar 관리 시스템이 완성되면, 해당 시스템을 통해 적절한 데이터를 불러 올 것.
        result = system->set3DSettings(1.f, 1.f, 1.f);
        if (result != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Fatal, "Failed to setup default 3D settings: {}", FMOD_ErrorString(result));
        }
    }

    AudioSystem::~AudioSystem()
    {
        if (system != nullptr)
        {
            FMOD_RESULT result = system->close();
            if (result != FMOD_OK)
            {
                IG_LOG(AudioSystemLog, Fatal, "Failed to close FMOD system: {}", FMOD_ErrorString(result));
            }
            result = system->release();
            if (result != FMOD_OK)
            {
                IG_LOG(AudioSystemLog, Fatal, "Failed to release FMOD system: {}", FMOD_ErrorString(result));
            }
        }
    }

    Handle<Audio> AudioSystem::CreateAudio(const std::string_view path)
    {
        IG_CHECK(system != nullptr);

        FMOD::Sound* newSound = nullptr;
        if (const FMOD_RESULT result = system->createSound(path.data(), FMOD_LOOP_OFF, nullptr, &newSound);
            result != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Warning, "Failed to create audio clip from path({})=>\n {}", path, FMOD_ErrorString(result));
            return {};
        }
        IG_CHECK(newSound != nullptr);

        ReadWriteLock lock{audioClipStorageMutex};
        return Handle<Audio>{audioClipStorage.Create(newSound).Value};
    }

    void AudioSystem::Destroy(const Handle<Audio> audioHandle)
    {
        IG_CHECK(system != nullptr);
        if (!audioHandle)
        {
            IG_LOG(AudioSystemLog, Warning, "Trying to destroy invalid audio clip.");
            return;
        }

        const auto soundHandle = Handle<FMOD::Sound*>{audioHandle.Value};
        FMOD::Sound* sound = nullptr;
        {
            ReadWriteLock lock{audioClipStorageMutex};
            FMOD::Sound** soundPtr = audioClipStorage.Lookup(soundHandle);
            if (soundPtr == nullptr || *soundPtr == nullptr)
            {
                IG_LOG(AudioSystemLog, Warning, "Trying to destroy unknown audio clip.");
                return;
            }
            sound = *soundPtr;
            audioClipStorage.Destroy(soundHandle);
        }
        IG_CHECK(sound != nullptr);

        if (const FMOD_RESULT result = sound->release();
            result != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Warning, "Failed to destroy audio clip.: {}", FMOD_ErrorString(result));
        }
    }

    void AudioSystem::Update(Registry& registry, const float deltaTime)
    {
        ZoneScopedN("AudioSystem.Update");
        IG_CHECK(system != nullptr);

        constexpr auto kCalcVelocity = [](const FMOD_VECTOR prevPos, const FMOD_VECTOR currentPos, const float dt)
        {
            if (dt <= FLT_EPSILON)
            {
                return FMOD_VECTOR{0.f, 0.f, 0.f};
            }
            const float invDeltaTime = 1.f / dt;
            const FMOD_VECTOR currentVelocity{
                .x = (currentPos.x - prevPos.x) * invDeltaTime,
                .y = (currentPos.y - prevPos.y) * invDeltaTime,
                .z = (currentPos.z - prevPos.z) * invDeltaTime
            };
            return currentVelocity;
        };

        const auto listenerView = registry.view<AudioListenerComponent, const TransformComponent>();
        system->set3DNumListeners((int)listenerView.size_hint());
        int listenerIdx = 0;
        for (auto [entity, listener, transform] : listenerView.each())
        {
            const FMOD_VECTOR prevPos{
                .x = listener.PrevPosition.x,
                .y = listener.PrevPosition.y,
                .z = listener.PrevPosition.z
            };
            const FMOD_VECTOR currentPos{
                .x = transform.Position.x,
                .y = transform.Position.y,
                .z = transform.Position.z
            };
            const FMOD_VECTOR currentVelocity = kCalcVelocity(prevPos, currentPos, deltaTime);
            const Vector3 forward = TransformUtility::MakeForward(transform);
            const Vector3 up = TransformUtility::MakeUp(transform);
            const FMOD_VECTOR fmodForward{
                .x = forward.x,
                .y = forward.y,
                .z = forward.z
            };
            const FMOD_VECTOR fmodUp{
                .x = up.x,
                .y = up.y,
                .z = up.z
            };
            system->set3DListenerAttributes(listenerIdx, &currentPos, &currentVelocity, &fmodForward, &fmodUp);
            listener.PrevPosition = transform.Position;
            ++listenerIdx;
        }

        ReadOnlyLock lock{audioClipStorageMutex};
        for (auto [entity, audioSource] : registry.view<AudioSourceComponent>(entt::exclude_t<TransformComponent>{}).each())
        {
            /*
             * 처리 결과에 따라 현재 프레임에 프로퍼티 값을 반영할지 하지 않을지 결정
             * 만약 당장 프로퍼티를 반영할 수 없다면, 다음 프레임으로 지연
             */
            if (FMOD::Channel* channel = UpdateAudioStatusUnsafe(entity, audioSource);
                channel != nullptr)
            {
                channel->setVolume(audioSource.Volume);
                channel->setPitch(audioSource.Pitch);
                channel->setPan(audioSource.Pan);
                channel->setMute(audioSource.bMute);
                FMOD_MODE newMode = FMOD_2D;
                newMode |= audioSource.bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
                channel->setMode(newMode);
            }
        }

        for (auto [entity, transform, audioSource] : registry.view<TransformComponent, AudioSourceComponent>().each())
        {
            if (FMOD::Channel* channel = UpdateAudioStatusUnsafe(entity, audioSource);
                channel != nullptr)
            {
                channel->setVolume(audioSource.Volume);
                channel->setPitch(audioSource.Pitch);
                channel->setPan(audioSource.Pan);
                channel->setMute(audioSource.bMute);
                channel->set3DMinMaxDistance(audioSource.MinDistance, audioSource.MaxDistance);
                FMOD_MODE newMode = FMOD_3D;
                newMode |= audioSource.bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
                channel->setMode(newMode);

                if (audioSource.LatestStatus == EAudioStatus::Playing)
                {
                    IG_CHECK(channel != nullptr);
                    const FMOD_VECTOR prevPos{
                        audioSource.PrevPosition.x,
                        audioSource.PrevPosition.y,
                        audioSource.PrevPosition.z
                    };
                    const FMOD_VECTOR currentPos{
                        .x = transform.Position.x,
                        .y = transform.Position.y,
                        .z = transform.Position.z
                    };
                    const FMOD_VECTOR currentVelocity = kCalcVelocity(prevPos, currentPos, deltaTime);
                    channel->set3DAttributes(&currentPos, &currentVelocity);
                }
                audioSource.PrevPosition = transform.Position;
            }
        }

        if (const FMOD_RESULT updateResult = system->update();
            updateResult != FMOD_OK)
        {
            IG_LOG(AudioSystemLog, Warning, "Failed to update audio system: {}.", FMOD_ErrorString(updateResult));
        }
    }

    FMOD::Channel* AudioSystem::UpdateAudioStatusUnsafe(const Entity entity, AudioSourceComponent& audioSource)
    {
        FMOD::Channel* channel = nullptr;
        bool bIsPlaying = false;                      /* FMOD에선 bIsPlaying이면 재생중이거나 중지(Paused)된 경우이다 */
        bool bIsPaused = false;                       /* bIsPaused이면 bIsPlaying */
        bool bShouldEraseChannelRelationship = false; /* 해당 프레임에서 이미 할당된 채널이 만료된 경우, 맵에서의 제거를 지연시켜 재활용 가능하도록 하기 위함 */
        const auto channelItr = channelMap.find(entity);
        if (channelItr != channelMap.end())
        {
            channel = channelItr->second;
            IG_CHECK(channel != nullptr);
            if ((channel->isPlaying(&bIsPlaying) != FMOD_OK) || !bIsPlaying)
            {
                bShouldEraseChannelRelationship = true;
                channel = nullptr;
            }
            else
            {
                if (channel->getPaused(&bIsPaused) != FMOD_OK)
                {
                    IG_CHECK_NO_ENTRY();
                }
            }
        }

        /*
         * Stopped -> Playing
         * Stopped -> Paused
         * */
        if (audioSource.NextEvent == EAudioEvent::Play || audioSource.NextEvent == EAudioEvent::PlayAsPaused)
        {
            /* 채널이 유효하지 않거나 존재하지 않기 때문에 새로운 채널 할당이 필요로함 */
            if (!bIsPlaying)
            {
                IG_CHECK(channel == nullptr);

                const AudioClip* audioClip = audioSource.Clip ?
                                                 Engine::GetAssetManager().Lookup(audioSource.Clip) :
                                                 nullptr;
                const Handle<Audio> audioHandle = audioClip != nullptr ?
                                                      audioClip->GetAudio() :
                                                      Handle<Audio>{};
                if (FMOD::Sound* sound = LookupUnsafe(audioHandle);
                    sound != nullptr)
                {
                    if (const FMOD_RESULT result = system->playSound(sound, nullptr, audioSource.NextEvent == EAudioEvent::PlayAsPaused, &channel);
                        result != FMOD_OK)
                    {
                        IG_LOG(AudioSystemLog, Warning, "Failed to play audio source.: {}", FMOD_ErrorString(result));
                    }
                    else
                    {
                        IG_CHECK(channel != nullptr);
                        /*  맵 슬롯을 재활용 가능하다면, 재활용 하기 */
                        if (bShouldEraseChannelRelationship)
                        {
                            channelItr->second = channel;
                            bShouldEraseChannelRelationship = false;
                        }
                        else
                        {
                            channelMap[entity] = channel;
                        }

                        audioSource.LatestStatus = audioSource.NextEvent == EAudioEvent::Play ?
                                                       EAudioStatus::Playing :
                                                       EAudioStatus::Paused;
                    }

                    audioSource.bShouldUpdatePropertiesOnThisFrame = true;
                }
            }
        }
        /*
         * Playing -> Paused
         */
        else if (audioSource.NextEvent == EAudioEvent::Pause)
        {
            if (bIsPlaying && !bIsPaused)
            {
                IG_CHECK(channel != nullptr);
                channel->setPaused(true);
                bIsPaused = true;
                audioSource.LatestStatus = EAudioStatus::Paused;
            }
        }
        /*
         * Paused -> Playing
         */
        else if (audioSource.NextEvent == EAudioEvent::Resume)
        {
            if (bIsPlaying && bIsPaused)
            {
                IG_CHECK(channel != nullptr);
                channel->setPaused(false);
                bIsPaused = false;
                audioSource.LatestStatus = EAudioStatus::Playing;
            }
        }
        /*
         * Playing -> Stopped
         * Paused -> Stopped
         */
        else if (audioSource.NextEvent == EAudioEvent::Stop)
        {
            /* 현재 프레임에선 FMOD API를 통해 채널을 멈춰주기만 하면, 다음 프레임에서 처리된다. */
            if (bIsPlaying)
            {
                IG_CHECK(channel != nullptr);
                channel->stop();
                bIsPlaying = false;
                bIsPaused = false;
                audioSource.LatestStatus = EAudioStatus::Stopped;
            }
        }
        audioSource.NextEvent = EAudioEvent::None;

        if (bShouldEraseChannelRelationship)
        {
            IG_CHECK(channel == nullptr);
            IG_CHECK(channelItr != channelMap.end());
            channelMap.erase(channelItr);
        }

        return channel;
    }

    FMOD::Sound* AudioSystem::LookupUnsafe(const Handle<Audio> audio)
    {
        FMOD::Sound** soundPtr = audioClipStorage.Lookup(Handle<FMOD::Sound*>{audio.Value});
        return soundPtr != nullptr ? *soundPtr : nullptr;
    }
}
