#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <TlHelp32.h>

#include <optional>
#include <vector>
#include <thread>
#include <mutex>

namespace Carbon {
	// Manages the game process including starting and stopping it
	// along with monitoring the game's executable and loaded modules
	class GameManager {
	public:
		// Initializes the game manager and starts a thread
		// listening for the game process
		GameManager();
		~GameManager();

		// Checks if the game is running
		// @return True if the game is running, false otherwise
		bool IsGameRunning();

		// Injects a module into the game process
		// @param modulePath The path to the module to inject
		void InjectModule(const std::string& modulePath);

		// Starts the game and waits for it to be running
		// This function will block until the game is running
		// This function spawns a new thread to actually start the game process
		void StartGame();

		// Stops the game forcefully
		void KillGame();

		// Checks if a module is loaded in the game process
		// @param moduleName The name of the module to check
		// @return True if the module is loaded, false otherwise
		bool IsModuleLoaded(const std::string& moduleName);

		// Gets all the loaded custom modules
		// @return A vector of all the loaded custom modules (mods injected via CarbonLauncher)
		std::vector<std::string> GetLoadedCustomModules();

	private:
		// Checks every 1s if the game is running
		std::thread gameStatusThread;

		// Mutex to protect the game status
		std::mutex gameStatusMutex;

		// Checks every 1s for game injected modules
		std::thread moduleHandlerThread;

		// Tracks when the game was first seen as running by the GameManager
		std::optional<std::chrono::time_point<std::chrono::system_clock>> gameStartedTime;

		// A list of all the loaded modules in the game process
		std::vector<MODULEENTRY32> modules;
		bool gameRunning = false;

		// The PID of the game process
		DWORD pid = 0;
	};
}; // namespace Carbon
