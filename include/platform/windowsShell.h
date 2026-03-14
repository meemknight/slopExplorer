#pragma once

namespace windowsShell
{
	bool acquireSingleInstance();
	void releaseSingleInstance();

	bool initTrayIcon();
	void shutdownTrayIcon();
	void pumpMessages();
	void waitForMessagesOrTimeout(double timeoutSeconds);

	bool consumeCreateWindowRequest();
	bool consumeQuitRequest();
}
