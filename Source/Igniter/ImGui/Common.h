#pragma once
#include <Core/Win32API.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);