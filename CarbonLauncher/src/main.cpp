#include "state.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h> 

#include <iostream>

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
	// Tell windows this program isn't important, so other processes (i.e. like the game) can have more resources
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	C.guiManager.RenderCallback(_GUI);
	C.guiManager.Run();

	FreeConsole();

	return 0;
}