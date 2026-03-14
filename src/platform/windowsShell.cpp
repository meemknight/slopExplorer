#include "windowsShell.h"

#ifdef _WIN32
	#include <Windows.h>
	#include <shellapi.h>
	#include <cstring>
#else
	#include <chrono>
	#include <thread>
#endif

#ifdef _WIN32
namespace
{
	constexpr const char *singleInstanceMutexName = "Local\\fileExplorer_single_instance_mutex";
	constexpr const char *trayWindowClassName = "fileExplorer_tray_window_class";
	constexpr UINT trayCallbackMessage = WM_APP + 1;
	constexpr UINT taskbarCreatedMessageInvalid = 0;
	constexpr UINT trayMenuCommandNewWindow = 1001;
	constexpr UINT trayMenuCommandQuit = 1002;

	HANDLE singleInstanceMutex = nullptr;
	HWND trayWindow = nullptr;
	UINT taskbarCreatedMessage = taskbarCreatedMessageInvalid;
	bool trayIconAdded = false;
	bool pendingCreateWindowRequest = false;
	bool pendingQuitRequest = false;

	void addTrayIcon()
	{
		if (trayWindow == nullptr)
		{
			return;
		}

		NOTIFYICONDATAA trayIconData = {};
		trayIconData.cbSize = sizeof(trayIconData);
		trayIconData.hWnd = trayWindow;
		trayIconData.uID = 1;
		trayIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		trayIconData.uCallbackMessage = trayCallbackMessage;
		trayIconData.hIcon = static_cast<HICON>(LoadImageA(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_SHARED));
		strncpy_s(trayIconData.szTip, "fileExplorer", _TRUNCATE);

		trayIconAdded = Shell_NotifyIconA(NIM_ADD, &trayIconData) == TRUE;
	}

	void showTrayMenu()
	{
		if (trayWindow == nullptr)
		{
			return;
		}

		POINT cursorPosition = {};
		GetCursorPos(&cursorPosition);

		HMENU menu = CreatePopupMenu();
		if (menu == nullptr)
		{
			return;
		}

		AppendMenuA(menu, MF_STRING, trayMenuCommandNewWindow, "New Window");
		AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
		AppendMenuA(menu, MF_STRING, trayMenuCommandQuit, "Quit");

		SetForegroundWindow(trayWindow);
		const UINT clickedCommand = TrackPopupMenu(menu,
			TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
			cursorPosition.x, cursorPosition.y, 0, trayWindow, nullptr);

		if (clickedCommand == trayMenuCommandNewWindow)
		{
			pendingCreateWindowRequest = true;
		}
		else if (clickedCommand == trayMenuCommandQuit)
		{
			pendingQuitRequest = true;
		}

		DestroyMenu(menu);
	}

	void removeTrayIcon()
	{
		if (!trayIconAdded || trayWindow == nullptr)
		{
			return;
		}

		NOTIFYICONDATAA trayIconData = {};
		trayIconData.cbSize = sizeof(trayIconData);
		trayIconData.hWnd = trayWindow;
		trayIconData.uID = 1;
		Shell_NotifyIconA(NIM_DELETE, &trayIconData);
		trayIconAdded = false;
	}

	LRESULT CALLBACK trayWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == taskbarCreatedMessage && taskbarCreatedMessage != taskbarCreatedMessageInvalid)
		{
			addTrayIcon();
			return 0;
		}

			switch (message)
			{
				case trayCallbackMessage:
				{
					if (wParam == 1 && (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK))
					{
						const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
						if (ctrlDown)
					{
						pendingQuitRequest = true;
					}
					else
					{
						pendingCreateWindowRequest = true;
					}
						return 0;
					}

					if (wParam == 1 && (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU))
					{
						showTrayMenu();
						return 0;
					}
					break;
				}

			case WM_DESTROY:
				removeTrayIcon();
				return 0;
		}

		return DefWindowProcA(hwnd, message, wParam, lParam);
	}
}

namespace windowsShell
{
	bool acquireSingleInstance()
	{
		if (singleInstanceMutex != nullptr)
		{
			return true;
		}

		singleInstanceMutex = CreateMutexA(nullptr, FALSE, singleInstanceMutexName);
		if (singleInstanceMutex == nullptr)
		{
			return false;
		}

		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(singleInstanceMutex);
			singleInstanceMutex = nullptr;
			return false;
		}

		return true;
	}

	void releaseSingleInstance()
	{
		if (singleInstanceMutex != nullptr)
		{
			CloseHandle(singleInstanceMutex);
			singleInstanceMutex = nullptr;
		}
	}

	bool initTrayIcon()
	{
		if (trayWindow != nullptr)
		{
			return true;
		}

		HINSTANCE instance = GetModuleHandleA(nullptr);
		WNDCLASSEXA windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = trayWindowProc;
		windowClass.hInstance = instance;
		windowClass.lpszClassName = trayWindowClassName;

		RegisterClassExA(&windowClass);
		trayWindow = CreateWindowExA(0, trayWindowClassName, "fileExplorerTrayWindow", 0,
			0, 0, 0, 0, nullptr, nullptr, instance, nullptr);

		if (trayWindow == nullptr)
		{
			return false;
		}

		taskbarCreatedMessage = RegisterWindowMessageA("TaskbarCreated");
		addTrayIcon();
		return trayIconAdded;
	}

	void shutdownTrayIcon()
	{
		removeTrayIcon();

		if (trayWindow != nullptr)
		{
			DestroyWindow(trayWindow);
			trayWindow = nullptr;
		}
	}

	void pumpMessages()
	{
		if (trayWindow == nullptr)
		{
			return;
		}

		MSG message = {};
		while (PeekMessageA(&message, trayWindow, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}

	void waitForMessagesOrTimeout(double timeoutSeconds)
	{
		if (timeoutSeconds <= 0.0)
		{
			return;
		}

		DWORD timeoutMilliseconds = static_cast<DWORD>(timeoutSeconds * 1000.0);
		if (timeoutMilliseconds == 0)
		{
			timeoutMilliseconds = 1;
		}

		MsgWaitForMultipleObjectsEx(0, nullptr, timeoutMilliseconds, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	}

	bool consumeCreateWindowRequest()
	{
		const bool result = pendingCreateWindowRequest;
		pendingCreateWindowRequest = false;
		return result;
	}

	bool consumeQuitRequest()
	{
		const bool result = pendingQuitRequest;
		pendingQuitRequest = false;
		return result;
	}
}

#else

namespace windowsShell
{
	bool acquireSingleInstance() { return true; }
	void releaseSingleInstance() {}
	bool initTrayIcon() { return true; }
	void shutdownTrayIcon() {}
	void pumpMessages() {}
	void waitForMessagesOrTimeout(double timeoutSeconds)
	{
		if (timeoutSeconds > 0.0)
		{
			std::this_thread::sleep_for(std::chrono::duration<double>(timeoutSeconds));
		}
	}
	bool consumeCreateWindowRequest() { return false; }
	bool consumeQuitRequest() { return false; }
}

#endif
