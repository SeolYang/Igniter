#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"

namespace ig
{
    struct TransformComponent;
}

namespace ig::ImGuiX
{
    template <typename E>
        requires std::is_enum_v<E>
    bool BeginEnumCombo(const std::string_view name, int& selectedIdx)
    {
        constexpr auto& EnumNames = magic_enum::enum_names<E>();
        if (!ImGui::BeginCombo(name.data(), EnumNames[selectedIdx].data()))
        {
            return false;
        }

        for (int idx = 0; idx < EnumNames.size(); ++idx)
        {
            if (ImGui::Selectable(EnumNames[idx].data(), selectedIdx == idx))
            {
                selectedIdx = idx;
                break;
            }
        }

        return true;
    }

    template <typename E>
        requires std::is_enum_v<E>
    bool BeginEnumCombo(const std::string_view name, E& enumValue)
    {
        int selected = 0;
        const bool bSuccuess = BeginEnumCombo<E>(name, selected);
        if (bSuccuess)
        {
            enumValue = magic_enum::enum_value<E>(selected);
        }
        return bSuccuess;
    }

    inline void EndEnumCombo()
    {
        ImGui::EndCombo();
    }

    template <typename E, typename... Excludes>
        requires std::is_enum_v<E> && (std::is_same_v<E, Excludes> && ...)
    std::optional<E> EnumMenuItems(std::optional<E> lastSelection = std::nullopt, [[maybe_unused]] const Excludes... excludes)
    {
        constexpr auto& EnumNames = magic_enum::enum_names<E>();
        constexpr auto& EnumVals = magic_enum::enum_values<E>();
        static_assert(EnumNames.size() == EnumVals.size());
        std::optional<E> selection = std::nullopt;
        for (Size idx = 0; idx < EnumVals.size(); ++idx)
        {
            if (((EnumVals[idx] != excludes) && ...))
            {
                if (ImGui::MenuItem(EnumNames[idx].data(), nullptr, lastSelection ? (*lastSelection == EnumVals[idx]) : false))
                {
                    selection = EnumVals[idx];
                    break;
                }
            }
        }

        return selection;
    }

    bool EditVector3(const std::string_view label, Vector3& vector, const F32 speed, const std::string_view format, const bool bReadOnly = false);
    bool EditTransform(const std::string_view label, TransformComponent& transform);
    void SeparatorText(const std::string_view text);
    bool EditColor3(const std::string_view label, Color& color);
    bool EditColor4(const std::string_view label, Color& color);
    bool EditColor3(const std::string_view label, Vector3& color);
    bool EditColor4(const std::string_view label, Vector4& color);

    inline void PushPositiveButtonHighlightColor()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImColor{73, 133, 204, 255}.Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor{27, 128, 247, 255}.Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor{16, 39, 61, 255}.Value);
    }

    inline void PopPositiveButtonHighlightColor()
    {
        ImGui::PopStyleColor(3);
    }

    inline void PushNegativeButtonHighlightColor()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImColor{202, 90, 90, 255}.Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor{255, 0, 0, 255}.Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor{61, 16, 16, 255}.Value);
    }

    inline void PopNegativeButtonHighlightColor()
    {
        ImGui::PopStyleColor(3);
    }

    ImVec2 GetFramePadding();

    void SetupDefaultTheme();
    void SetupTransparentTheme(bool bStyleDark_, F32 alpha_);
} // namespace ig::ImGuiX
