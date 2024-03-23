#pragma once
#include <ImGui/ImGuiLayer.h>

namespace ig
{
    class CachedStringDebugger : public ImGuiLayer
    {
    public:
        CachedStringDebugger()= default;
        ~CachedStringDebugger() = default;

        void Render() override;

    private:
        constexpr static size_t MaxInputLength = 32;
        std::string inputBuffer{ MaxInputLength };
        std::vector<std::pair<uint64_t, std::string_view>> cachedStrings;
        bool bSortRequired = true;
        bool bLoadRequired = true;
        bool bFiltered = false;

        size_t searchTargetHashVal = InvalidHashVal;
    };
} // namespace ig