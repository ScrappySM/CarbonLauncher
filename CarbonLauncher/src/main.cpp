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
	// Tell winapi to show a console window
	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);

    auto console = spdlog::stdout_color_mt("carbon");      // Create a new logger with color support
	spdlog::set_default_logger(console);                   // Set the default logger to the console logger
    console->set_level(spdlog::level::trace);			   // Set the log level to info
	spdlog::set_pattern("%^[ %H:%M:%S |  %-8l] %n: %v%$"); // Nice log format

	// Tell windows this program isn't important, so other processes can use more resources
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	C.guiManager.RenderCallback(_GUI);
	C.guiManager.Run();

	FreeConsole();

	return 0;
}