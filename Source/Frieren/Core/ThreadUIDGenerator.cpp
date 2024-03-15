#include <Core/ThreadUIDGenerator.h>
#include <atomic>

namespace fe
{
#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
    std::atomic_size_t uid{ 0 };
#pragma warning(pop)

    size_t ThreadUIDGenerator::GetUID()
    {
        const static thread_local size_t tuid = uid.fetch_add(1);
        return tuid;
    }
} // namespace fe