#pragma once

#include <unordered_map>

namespace fileExplorer
{
	struct fileExplorerWindow
	{
		bool open = true;
		bool showHiddenFiles = false;
		bool gridView = false;
		float leftPaneWidth = 220.0f;
		int selectedFolder = 0;
		char currentPath[512] = "C:\\";
		char searchFilter[128] = {};
	};

	void createWindow(std::unordered_map<int, fileExplorerWindow> &windows, int &nextWindowId);
	bool createWindowShortcutPressed();
	void drawWindows(std::unordered_map<int, fileExplorerWindow> &windows, int &nextWindowId);
	void shutdownWindowShortcuts();
	float &globalFontScale();
}
