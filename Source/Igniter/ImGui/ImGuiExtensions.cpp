#include <Igniter.h>
#include <imgui_internal.h>
#include <Component/TransformComponent.h>
#include <ImGui/ImGuiExtensions.h>

namespace ig::ImGuiX
{
    bool EditVector3(const std::string_view label, Vector3& vector, const float speed, const std::string_view format)
    {
        bool bModified = false;
        constexpr ImColor XComponentColor{ 0.8f, 0.15f, 0.f, 1.f };
        constexpr ImColor YComponentColor{ 0.404f, 0.66f, 0.f, 1.f };
        constexpr ImColor ZComponentColor{ 0.172f, 0.5f, 0.93f, 1.f };
        constexpr auto CalculateColorBoxRect = [](const float width, const float offset, const ImVec2 cursorPos, const ImVec2 labelSize, const ImVec2 framePadding)
        {
            return ImRect{
                ImVec2{cursorPos.x + offset, cursorPos.y + labelSize.y * 0.35f },
                ImVec2{cursorPos.x + offset + width, cursorPos.y + labelSize.y * 0.65f + framePadding.y * 2.f } };
        };

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 labelSize = ImGui::CalcTextSize(format.data(), nullptr, true);

        ImGui::BeginGroup();
        ImGui::PushID(label.data());
        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        constexpr float ColorBoxOffset = 5.5f;
        constexpr float ColorBoxWidth = 3.5f;
        constexpr float ColorBoxRounding = 3.5f;
        ImGui::PushID(0);
        {
            const ImRect colorBoxRect{ CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding) };
            bModified |= ImGui::DragFloat("", &vector.x, speed, 0.f, 0.f, format.data());
            window->DrawList->AddRectFilled(colorBoxRect.Min, colorBoxRect.Max, XComponentColor, ColorBoxRounding);
        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::PushID(1);
        {
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            const ImRect colorBoxRect{ CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding) };
            bModified |= ImGui::DragFloat("", &vector.y, speed, 0.f, 0.f, format.data());
            window->DrawList->AddRectFilled(colorBoxRect.Min, colorBoxRect.Max, YComponentColor, ColorBoxRounding);

        }
        ImGui::PopID();
        ImGui::PopItemWidth();

        ImGui::PushID(2);
        {
            ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
            const ImRect colorBoxRect{ CalculateColorBoxRect(ColorBoxWidth, ColorBoxOffset, window->DC.CursorPos, labelSize, style.FramePadding) };
            bModified |= ImGui::DragFloat("", &vector.z, speed, 0.f, 0.f, format.data());
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
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4{ 0.05f, 0.05f, 0.05f, 1.f });
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4{ 0.05f, 0.05f, 0.05f, 1.f });
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
                transform.Rotation = Quaternion::CreateFromYawPitchRoll(Vector3{
                    Deg2Rad(eulerAngles.x),
                    Deg2Rad(eulerAngles.y),
                    Deg2Rad(eulerAngles.z) });
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
        constexpr float SeparatorThickness = 1.0f;
        constexpr float SeparatorOffset = 10.f;
        const ImVec2 textSize{ ImGui::CalcTextSize(text.data()) };
        ImGui::Text(text.data());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SeparatorOffset);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textSize.y * 0.5f - SeparatorThickness * 0.5f);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, SeparatorThickness);
    }

    bool EditColor3(const std::string_view label, Color& color)
    {
        /* #sy_todo 사이즈 조절 */
        float colorVal[3]{ color.R(), color.G(), color.B() };
        if (ImGui::ColorEdit3(label.data(), colorVal, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs))
        {
            color.x = colorVal[0];
            color.y = colorVal[1];
            color.z = colorVal[2];
            return true;
        }

        return false;
    }

    bool EditColor4(const std::string_view label, Color& color)
    {
        /* #sy_todo 사이즈 조절 (원본 구현 참고) */
        float colorVal[4]{ color.R(), color.G(), color.B(), color.A()};
        if (ImGui::ColorEdit4(label.data(), colorVal, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs))
        {
            color.x = colorVal[0];
            color.y = colorVal[1];
            color.z = colorVal[2];
            color.w = colorVal[3];
            return true;
        }

        return false;
    }
}