#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/HandleStorage.h"

namespace ig
{
    constexpr U32 kMaxNumLights = 32768;

    template <typename Ty>
    struct InFlightFramesResource
    {
      public:
        [[nodiscard]] Ty& operator[](const LocalFrameIndex localFrameIdx) noexcept { return Resources[localFrameIdx]; }
        [[nodiscard]] const Ty& operator[](const LocalFrameIndex localFrameIdx) const noexcept { return Resources[localFrameIdx]; }

      public:
        eastl::array<Ty, NumFramesInFlight> Resources{};
    };

    // 각 타입에 정의하고, 각 타입에 대해 Create/Destroy에서 rw lock, Lookup 에선 read only lock
    // 각 방식(GpuViewManager,..) 내부 구현이 Thread Safe를 보장하지 않아도 됨
    // 할당은 여러 스레드에서 이뤄질 수 있지만, 해제는 결국 메인 스레드에서 진행됨
    template <typename Ty>
    struct DeferredResourceManagePackage
    {
      public:
        mutable SharedMutex StorageMutex;
        HandleStorage<Ty> Storage;
        InFlightFramesResource<Mutex> DeferredDestroyPendingListMutex;
        InFlightFramesResource<eastl::vector<Handle<Ty>>> DeferredDestroyPendingList;
    };
} // namespace ig
