#pragma once
#include <Igniter.h>
#include <Core/String.h>

namespace ig
{
    template <typename E>
        requires std::is_enum_v<E>
    bool BeginEnumCombo(const std::string_view name, int& selectedIdx)
    {
        constexpr auto EnumNames = magic_enum::enum_names<E>();
        if (!ImGui::BeginCombo(name.data(), EnumNames[selectedIdx].data()))
        {
            return false;
        }

        for (int idx = 0; idx < EnumNames.size(); ++idx)
        {
            if (ImGui::Selectable(EnumNames[idx].data(), selectedIdx == idx))
            {
                selectedIdx = idx;
            }
        }

        return true;
    }

    void EndEnumCombo()
    {
        ImGui::EndCombo();
    }
} // namespace ig