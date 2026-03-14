#include "fileExplorerWindow.h"

#include "imgui.h"
#include <cstdio>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
constexpr int fileExplorerCreateWindowHotkeyId = 0x1200;
constexpr ImGuiID fileExplorerWindowClassId = 0xF11E0001;
	bool createWindowShortcutPressedFallback()
	{
		static bool wasPressed = false;
		const bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		const bool leftWinDown = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0;
		const bool rightWinDown = (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
		const bool eDown = (GetAsyncKeyState('E') & 0x8000) != 0;
		const bool isPressed = ctrlDown && (leftWinDown || rightWinDown) && eDown;
		const bool triggered = isPressed && !wasPressed;

		wasPressed = isPressed;
		return triggered;
	}
#endif

namespace
{
	void drawSingleWindow(int windowId, fileExplorer::fileExplorerWindow &windowData, bool &createWindowRequest)
	{
		char windowLabel[128] = {};
		snprintf(windowLabel, sizeof(windowLabel), "File Explorer %d###file_explorer_window_%d", windowId, windowId);

		ImGuiWindowClass explorerWindowClass = {};
		explorerWindowClass.ClassId = fileExplorerWindowClassId;
		explorerWindowClass.ParentViewportId = 0;
		explorerWindowClass.DockingAlwaysTabBar = true;
		ImGui::SetNextWindowClass(&explorerWindowClass);

		if (windowData.open)
		{
			ImGui::SetNextWindowSize({900, 600}, ImGuiCond_FirstUseEver);
		}

		const bool beginResult = ImGui::Begin(windowLabel, &windowData.open);
		if (ImGuiViewport *viewport = ImGui::GetWindowViewport())
		{
			windowData.nativeViewportId = viewport->ID;
			windowData.nativeWindowCloseRequested = viewport->PlatformRequestClose;
		}
		else
		{
			windowData.nativeViewportId = 0;
			windowData.nativeWindowCloseRequested = false;
		}

		if (beginResult)
		{
			ImGui::PushID(windowId);

			float &fontScale = fileExplorer::globalFontScale();
			ImGui::SliderFloat("Global Font Scale", &fontScale, 0.75f, 3.5f, "%.2f");
			ImGui::TextUnformatted("Applies to all explorer windows.");
			ImGui::Separator();

			if (ImGui::Button("New Explorer"))
			{
				createWindowRequest = true;
			}

			ImGui::SameLine();
			ImGui::Text("Window ID: %d", windowId);

			ImGui::InputText("Current Path", windowData.currentPath, IM_ARRAYSIZE(windowData.currentPath));
			ImGui::InputText("Search", windowData.searchFilter, IM_ARRAYSIZE(windowData.searchFilter));
			ImGui::Checkbox("Show hidden files", &windowData.showHiddenFiles);
			ImGui::SameLine();
			ImGui::Checkbox("Grid view", &windowData.gridView);
			ImGui::SliderFloat("Left pane width", &windowData.leftPaneWidth, 140.0f, 420.0f);

			ImGui::Separator();

			ImGui::BeginChild("folders_panel", {windowData.leftPaneWidth, 0.0f}, true);
			ImGui::TextUnformatted("Folders");
			ImGui::Separator();

			static const char *folderNames[] =
			{
				"Desktop",
				"Documents",
				"Downloads",
				"Pictures",
				"Projects",
				"Temp"
			};

			if (windowData.selectedFolder < 0 || windowData.selectedFolder >= IM_ARRAYSIZE(folderNames))
			{
				windowData.selectedFolder = 0;
			}

			for (int i = 0; i < IM_ARRAYSIZE(folderNames); i++)
			{
				if (ImGui::Selectable(folderNames[i], windowData.selectedFolder == i))
				{
					windowData.selectedFolder = i;
				}
			}

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("files_panel", {0.0f, 0.0f}, true);
			ImGui::Text("Placeholder contents for: %s", folderNames[windowData.selectedFolder]);
			ImGui::Separator();
			ImGui::BulletText("file_a.txt");
			ImGui::BulletText("file_b.png");
			ImGui::BulletText("notes.md");
			ImGui::BulletText("archive.zip");
			ImGui::Spacing();
			ImGui::TextUnformatted("Drop placeholders: folders, files, and tabs can be wired next.");
			ImGui::EndChild();

			ImGui::PopID();
		}

		ImGui::End();
	}
}

namespace fileExplorer
{
	float &globalFontScale()
	{
		static float fontScale = 2.0f;
		return fontScale;
	}

	void createWindow(std::unordered_map<int, fileExplorerWindow> &windows, int &nextWindowId)
	{
		windows[nextWindowId] = fileExplorerWindow{};
		nextWindowId += 1;
	}

	bool createWindowShortcutPressed()
	{
#ifdef _WIN32
		static bool hotkeyRegistrationAttempted = false;
		static bool hotkeyRegistered = false;

		if (!hotkeyRegistrationAttempted)
		{
			hotkeyRegistrationAttempted = true;
			hotkeyRegistered = RegisterHotKey(nullptr, fileExplorerCreateWindowHotkeyId,
				MOD_CONTROL | MOD_WIN | MOD_NOREPEAT, 'E') != 0;
		}

		if (hotkeyRegistered)
		{
			MSG message = {};
			while (PeekMessage(&message, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE))
			{
				if (message.message == WM_HOTKEY && message.wParam == fileExplorerCreateWindowHotkeyId)
				{
					return true;
				}
			}
		}

		return createWindowShortcutPressedFallback();
#else
		return false;
#endif
	}

	void drawWindows(std::unordered_map<int, fileExplorerWindow> &windows, int &nextWindowId)
	{
		bool createWindowRequest = false;
		std::vector<int> windowsToClose;
		std::vector<unsigned int> nativeViewportsToClose;

		for (auto &windowEntry : windows)
		{
			const bool wasOpen = windowEntry.second.open;
			drawSingleWindow(windowEntry.first, windowEntry.second, createWindowRequest);

			if (!windowEntry.second.open)
			{
				windowsToClose.push_back(windowEntry.first);

				if (wasOpen && windowEntry.second.nativeWindowCloseRequested && windowEntry.second.nativeViewportId != 0)
				{
					nativeViewportsToClose.push_back(windowEntry.second.nativeViewportId);
				}
			}
		}

		for (const auto &windowEntry : windows)
		{
			if (!windowEntry.second.open)
			{
				continue;
			}

			for (unsigned int nativeViewportId : nativeViewportsToClose)
			{
				if (windowEntry.second.nativeViewportId == nativeViewportId)
				{
					windowsToClose.push_back(windowEntry.first);
					break;
				}
			}
		}

		std::sort(windowsToClose.begin(), windowsToClose.end());
		windowsToClose.erase(std::unique(windowsToClose.begin(), windowsToClose.end()), windowsToClose.end());

		for (int closedWindowId : windowsToClose)
		{
			windows.erase(closedWindowId);
		}

		if (createWindowRequest)
		{
			createWindow(windows, nextWindowId);
		}
	}

	void shutdownWindowShortcuts()
	{
#ifdef _WIN32
		UnregisterHotKey(nullptr, fileExplorerCreateWindowHotkeyId);
#endif
	}
}
