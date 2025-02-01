#include "Igniter/Igniter.h"
#include <imgui_internal.h>
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/ImGui/ImGuiExtensions.h"

namespace ig::ImGuiX
{
    ImVec2 GetFramePadding()
    {
        return ImGui::GetCurrentContext()->Style.FramePadding;
    }

    bool EditVector3(const std::string_view label, Vector3& vector, const F32 speed, const std::string_view format, const bool bReadOnly)
    {
        bool bModified = false;
        constexpr ImColor XComponentColor{0.8f, 0.15f, 0.f, 1.f};
        constexpr ImColor YComponentColor{0.404f, 0.66f, 0.f, 1.f};
        constexpr ImColor ZComponentColor{0.172f, 0.5f, 0.93f, 1.f};
        constexpr auto CalculateColorBoxRect =
            [](const F32 width, const F32 offset, const ImVec2 cursorPos, const ImVec2 labelSize, const ImVec2 framePadding)
        {
            return ImRect{
                ImVec2{cursorPos.x + offset, cursorPos.y + labelSize.y * 0.35f},
                ImVec2{cursorPos.x + offset + width, cursorPos.y + labelSize.y * 0.65f + framePadding.y * 2.f}};
        };

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 labelSize = ImGui::CalcTextSize(format.data(), nullptr, true);

        const ImGuiSliderFlags kSliderFlags = bReadOnly ? ImGuiSliderFlags_NoInput : ImGuiSliderFlags_None;

        ImGui::BeginGroup();
        ImGui::PushID(label.data());
        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        constexpr F32 ColorBoxOffset = 5.5f;
        constexpr F32 ColorBoxWidth = 3.5f;
        constexpr F32 ColorBoxRounding = 3.5f;
        ImGui::PushID(0);
        {
            const ImRect colorBoxRect{CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding)};
            bModified |= ImGui::DragFloat("", &vector.x, speed, 0.f, 0.f, format.data(), kSliderFlags);
            window->DrawList->AddRectFilled(colorBoxRect.Min, colorBoxRect.Max, XComponentColor, ColorBoxRounding);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::PushID(1);
        {
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            const ImRect colorBoxRect{CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding)};
            bModified |= ImGui::DragFloat("", &vector.y, speed, 0.f, 0.f, format.data(), kSliderFlags);
            window->DrawList->AddRectFilled(colorBoxRect.Min, colorBoxRect.Max, YComponentColor, ColorBoxRounding);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::PushID(2);
        {
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            const ImRect colorBoxRect{CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding)};
            bModified |= ImGui::DragFloat("", &vector.z, speed, 0.f, 0.f, format.data(), kSliderFlags);
            window->DrawList->AddRectFilled(colorBoxRect.Min, colorBoxRect.Max, ZComponentColor, ColorBoxRounding);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::EndGroup();

        return bModified;
    }

    bool EditTransform(const std::string_view label, TransformComponent& transform)
    {
        ImGui::PushID(label.data());
        bool bModifiedVal = false;
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4{0.05f, 0.05f, 0.05f, 1.f});
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4{0.05f, 0.05f, 0.05f, 1.f});
        if (ImGui::BeginTable("##TransformTable", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable))
        {
            ImGui::TableNextColumn();
            ImGui::Text("Position");
            ImGui::TableNextColumn();
            bModifiedVal |= EditVector3("EditPosition", transform.Position, 0.1f, "%.3f");
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Rotation");
            ImGui::TableNextColumn();
            Vector3 eulerAngles = transform.Rotation.ToEuler();
            eulerAngles.x = Rad2Deg(eulerAngles.x);
            eulerAngles.y = Rad2Deg(eulerAngles.y);
            eulerAngles.z = Rad2Deg(eulerAngles.z);
            if (EditVector3("EditRotation", eulerAngles, 0.5f, "%.3f°"))
            {
                transform.Rotation =
                    Quaternion::CreateFromYawPitchRoll(Vector3{Deg2Rad(eulerAngles.x), Deg2Rad(eulerAngles.y), Deg2Rad(eulerAngles.z)});
                bModifiedVal = true;
            }
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            bModifiedVal |= EditVector3("EditScale", transform.Scale, 0.001f, "%.3f");
            ImGui::EndTable();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        return bModifiedVal;
    }

    void SeparatorText(const std::string_view text)
    {
        constexpr F32 SeparatorThickness = 1.0f;
        constexpr F32 SeparatorOffset = 10.f;
        const ImVec2 textSize{ImGui::CalcTextSize(text.data())};
        ImGui::Text(text.data());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SeparatorOffset);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textSize.y * 0.5f - SeparatorThickness * 0.5f);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, SeparatorThickness);
    }

    inline bool EditColor3(const std::string_view label, F32* color)
    {
        /* #sy_todo 사이즈 조절 */
        return ImGui::ColorEdit3(label.data(), color, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
    }

    inline bool EditColor4(const std::string_view label, F32* color)
    {
        /* #sy_todo 사이즈 조절 */
        return ImGui::ColorEdit4(label.data(), color, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs);
    }

    bool EditColor3(const std::string_view label, Color& color)
    {
        return EditColor3(label, &color.x);
    }

    bool EditColor3(const std::string_view label, Vector3& color)
    {
        return EditColor3(label, &color.x);
    }

    bool EditColor4(const std::string_view label, Color& color)
    {
        return EditColor4(label, &color.x);
    }

    bool EditColor4(const std::string_view label, Vector4& color)
    {
        return EditColor4(label, &color.x);
    }

    void SetupDefaultTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.WindowPadding = ImVec2(10.0f, 8.0f);
        style.WindowRounding = 5.0f;
        style.WindowBorderSize = 0.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.ChildRounding = 5.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 5.0f;
        style.PopupBorderSize = 0.0f;
        style.FramePadding = ImVec2(10.0f, 5.0f);
        style.FrameRounding = 5.0f;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(7.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(2.0f, 4.0f);
        style.CellPadding = ImVec2(7.0f, 2.0f);
        style.IndentSpacing = 1.5f;
        style.ColumnsMinSpacing = 7.0f;
        style.ScrollbarSize = 12.5f;
        style.ScrollbarRounding = 20.0f;
        style.GrabMinSize = 6.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 4.0f;
        style.TabBorderSize = 0.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style.Colors[ImGuiCol_Text] = ImVec4(0.8627451062202454f, 0.8823529481887817f, 0.8980392217636108f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6196078658103943f, 0.6196078658103943f, 0.6196078658103943f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2196078449487686f, 0.2196078449487686f, 0.2196078449487686f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.1411764770746231f, 0.1411764770746231f, 0.1411764770746231f, 1.0f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.09803921729326248f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3411764800548553f, 0.3411764800548553f, 0.3411764800548553f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.501960813999176f, 0.501960813999176f, 0.501960813999176f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.501960813999176f, 0.501960813999176f, 0.501960813999176f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.1392156898975372f, 0.2784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3411764800548553f, 0.3411764800548553f, 0.3411764800548553f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.8235294222831726f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3532207012176514f, 0.5698376893997192f, 0.8068669438362122f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4392156898975372f, 0.8784313797950745f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1294117718935013f, 0.1294117718935013f, 0.1294117718935013f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1843137294054031f, 0.1843137294054031f, 0.1843137294054031f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.1215686276555061f, 0.1215686276555061f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(1.0f, 1.0f, 1.0f, 0.105882354080677f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.08235294371843338f, 0.08235294371843338f, 0.08235294371843338f, 1.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.1019607856869698f, 0.1019607856869698f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 1.0f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.2039215713739395f, 0.2313725501298904f, 0.2823529541492462f, 0.7529411911964417f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.105882354080677f, 0.1137254908680916f, 0.1372549086809158f, 0.7529411911964417f);
    }

    void SetupTransparentTheme(bool bStyleDark_, F32 alpha_)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
        style.Alpha = 1.0f;
        style.FrameRounding = 3.0f;
        style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

        if (bStyleDark_)
        {
            for (int i = 0; i <= ImGuiCol_COUNT; i++)
            {
                ImVec4& col = style.Colors[i];
                F32 H, S, V;
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

                if (S < 0.1f)
                {
                    V = 1.0f - V;
                }
                ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
                if (col.w < 1.00f)
                {
                    col.w *= alpha_;
                }
            }
        }
        else
        {
            for (int i = 0; i <= ImGuiCol_COUNT; i++)
            {
                ImVec4& col = style.Colors[i];
                if (col.w < 1.00f)
                {
                    col.x *= alpha_;
                    col.y *= alpha_;
                    col.z *= alpha_;
                    col.w *= alpha_;
                }
            }
        }
    }
} // namespace ig::ImGuiX
