#define GLM_ENABLE_EXPERIMENTAL
#include "gameLayer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "platformInput.h"
#include "imgui.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include "imfilebrowser.h"
#include <gl2d/gl2d.h>
#include <platformTools.h>
#include <IconsForkAwesome.h>
#include <imguiTools.h>
#include <logs.h>
#include "fileExplorerWindow.h"
#include "windowsShell.h"


bool initGame()
{


	return true;
}


//IMPORTANT NOTICE, IF YOU WANT TO SHIP THE GAME TO ANOTHER PC READ THE README.MD IN THE GITHUB
//https://github.com/meemknight/cmakeSetup
//OR THE INSTRUCTION IN THE CMAKE FILE.
//YOU HAVE TO CHANGE A FLAG IN THE CMAKE SO THAT RESOURCES_PATH POINTS TO RELATIVE PATHS
//BECAUSE OF SOME CMAKE PROGBLMS, RESOURCES_PATH IS SET TO BE ABSOLUTE DURING PRODUCTION FOR MAKING IT EASIER.

bool gameLogic(float deltaTime, platform::Input &input)
{
#pragma region init stuff
	int w = 0; int h = 0;
	w = platform::getFrameBufferSizeX(); //window w
	h = platform::getFrameBufferSizeY(); //window h
	
	//glViewport(0, 0, w, h);
	//glClear(GL_COLOR_BUFFER_BIT); //clear screen
	//
	//renderer.updateWindowMetrics(w, h);
#pragma endregion

	//we will remove the normal window so only the imgui code remains
	//you can also do platform::isButtonHeld(platform::Button::Left)

	//if (input.isButtonHeld(platform::Button::Left))
	//{
	//	gameData.rectPos.x -= deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Right))
	//{
	//	gameData.rectPos.x += deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Up))
	//{
	//	gameData.rectPos.y -= deltaTime * 100;
	//}
	//if (input.isButtonHeld(platform::Button::Down))
	//{
	//	gameData.rectPos.y += deltaTime * 100;
	//}
	//
	//gameData.rectPos = glm::clamp(gameData.rectPos, glm::vec2{0,0}, glm::vec2{w - 100,h - 100});
	//renderer.renderRectangle({gameData.rectPos, 100, 100}, Colors_Blue);
	//
	//renderer.flush();


	static std::unordered_map<int, fileExplorer::fileExplorerWindow> explorerWindows;
	static int nextExplorerWindowId = 1000;
	static bool explorerWindowStateInitialized = false;
	const bool createInitialExplorerWindow = true;

	if (!explorerWindowStateInitialized && createInitialExplorerWindow)
	{
		fileExplorer::createWindow(explorerWindows, nextExplorerWindowId);
	}

	explorerWindowStateInitialized = true;

	if (fileExplorer::createWindowShortcutPressed())
	{
		fileExplorer::createWindow(explorerWindows, nextExplorerWindowId);
	}

	if (windowsShell::consumeCreateWindowRequest())
	{
		fileExplorer::createWindow(explorerWindows, nextExplorerWindowId);
	}

	if (windowsShell::consumeQuitRequest())
	{
		return false;
	}

	fileExplorer::drawWindows(explorerWindows, nextExplorerWindowId);

	//I don't want this feature!
	//if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	//{
	//	explorerWindows.clear();
	//}

	return true;
#pragma endregion

}

//This function might not be be called if the program is forced closed
void closeGame()
{
	fileExplorer::shutdownWindowShortcuts();

}
