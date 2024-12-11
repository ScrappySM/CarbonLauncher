#include "guimanager.h"
#include "discordmanager.h"
#include "gamemanager.h"
#include "pipemanager.h"
#include "state.h"

using namespace Carbon;

/*
 * Main entry point for the Carbon Launcher
 * 
 * @param hInstance The instance of the application
 * @param hPrevInstance The previous instance of the application
 * @param lpCmdLine The command line arguments
 * @param nCmdShow The command show
 * @return The exit code of the application
 */
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	auto consoleResult = AllocConsole();
	if (consoleResult) {
		FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);
	}
	else {
		MessageBox(NULL, L"Failed to allocate console", L"Error", MB_ICONERROR);
	}

	C.guiManager = new GUIManager(hInstance);
	C.discordManager = new DiscordManager();
	C.gameManager = new GameManager();
	C.pipeManager = new PipeManager();

	C.guiManager->RenderCallback(_GUI);
	C.guiManager->Run();

	FreeConsole();

	return 0;
}