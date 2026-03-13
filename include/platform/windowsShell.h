#pragma once

namespace windowsShell
{
	bool acquireSingleInstance();
	void releaseSingleInstance();

	bool initTrayIcon();
	void shutdownTrayIcon();
	void pumpMessages();

	bool consumeCreateWindowRequest();
	bool consumeQuitRequest();
}
