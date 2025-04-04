#include "Igniter/Igniter.h"
#include "Igniter/Core/Thread.h"

namespace ig
{
#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
    std::atomic_uint64_t uid{0};
    Size mainThreadUid{std::numeric_limits<Size>::max()};
#pragma warning(pop)

    Size ThreadUIDGenerator::GetUID()
    {
        const static thread_local Size tuid = uid.fetch_add(1);
        return tuid;
    }

    void ThreadInfo::RegisterMainThreadID()
    {
        mainThreadUid = ThreadUIDGenerator::GetUID();
    }

    bool ThreadInfo::IsMainThread()
    {
        IG_CHECK(mainThreadUid != std::numeric_limits<Size>::max() && "Main thread does not registered.");
        return ThreadUIDGenerator::GetUID() == mainThreadUid;
    }
} // namespace ig
