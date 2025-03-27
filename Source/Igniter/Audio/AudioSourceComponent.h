#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    class AudioClip;

    enum class EAudioStatus : U8
    {
        Playing,
        Paused,
        Stopped,
    };

    enum class EAudioEvent : U8
    {
        None,
        Play,
        PlayAsPaused,
        Pause,
        Resume,
        Stop,
    };

    class Audio;

    struct AudioSourceComponent
    {
        //Handle<AudioClip> Clip{};
        /* Test Purpose */
        Handle<Audio> Clip;

        /* Properties */
        float Volume = 0.5f;
        float Pitch = 1.f;
        float Pan = 0.f;
        bool bMute = false;
        bool bLoop = false;
        float MinDistance = 1.f;
        float MaxDistance = 10000.f;
        /* Per Frame Flags*/
        /*
         * 변경된 프로퍼티 값을 시스템에 반영 시키기 위해선 해당 플래그를 true로 변경해주어야 함.
         * 또는 System에 존재하는 함수를 사용하여 개별적으로 변경 할 수도 있다.
         */
        bool bShouldUpdatePropertiesOnThisFrame = false;

        /* 이전 프레임에서의 상태 (Readonly) */
        EAudioStatus LatestStatus = EAudioStatus::Stopped;
        /* 다음 프레임에서 반영할 상태(이벤트) */
        EAudioEvent NextEvent = EAudioEvent::None;

        Vector3 PrevPosition = Vector3::Zero;
    };

    template <>
    void OnInspector<AudioSourceComponent>(Registry* registry, const Entity entity);

    IG_META_DECLARE(AudioSourceComponent);
}
