#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#include "sm.h"
#include "utils.h"
#include "globals.h"

/*
 * DLL main thread
 *
 * @param lpParam The parameter, in this case the module handle
 */
DWORD WINAPI DllMainThread(LPVOID lpParam) {
	HMODULE hModule = static_cast<HMODULE>(lpParam); // Get the module handle to our DLL
	Utils::InitLogging(); // Setup our custom logging

	auto contraption = SM::Contraption::GetInstance(); // Contraption is the game's main singleton
	auto pipe = Utils::Pipe::GetInstance(); // Connection to launcher

	contraption->console->Hook(); // Hook the console, sending log messages to the launcher
	contraption->WaitForStateEgress(LOADING); // Wait for the game to finish loading

	pipe->SendPacket(LOADED); // Inform the launcher we have loaded

	// Whenever the state changes, send a packet informing the launcher
	contraption->OnStateChange([&pipe](ContraptionState state) {
		pipe->SendPacket(STATECHANGE, static_cast<int>(state));
		});

	// Quit the thread, we never get here because the game closes the process
	// but it's good practice to have a clean exit
	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

/*
 * DLL entry point
 *
 * @param hModule The module handle
 * @param ulReason The reason for calling this function
 * @param lpReserved Reserved
 * @return The exit code of the application
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
	if (ulReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, DllMainThread, hModule, 0, nullptr);
	}

	return TRUE;
}