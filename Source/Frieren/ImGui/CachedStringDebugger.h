#pragma once
#include <Frieren.h>
#include <Core/Hash.h>

namespace fe
{
    class CachedStringDebugger final
    {
    public:
        CachedStringDebugger() = default;
        CachedStringDebugger(const CachedStringDebugger&) = delete;
        CachedStringDebugger(CachedStringDebugger&&) noexcept = delete;
        ~CachedStringDebugger() = default;

        CachedStringDebugger& operator=(const CachedStringDebugger&) = delete;
        CachedStringDebugger& operator=(CachedStringDebugger&&) = delete;

        void OnImGui();

    private:
        constexpr static size_t MaxInputLength = 32;
        std::string inputBuffer{MaxInputLength};
        std::vector<std::pair<uint64_t, std::string_view>> cachedStrings{};
        bool bSortRequired = true;
        bool bLoadRequired = true;
        bool bFiltered = false;

        size_t searchTargetHashVal = InvalidHashVal;
    };
}    // namespace fe
