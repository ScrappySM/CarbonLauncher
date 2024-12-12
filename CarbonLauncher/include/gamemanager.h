#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <TlHelp32.h>

#include <optional>
#include <vector>
#include <thread>
#include <mutex>

namespace Carbon {
	class GameManager {
	public:
		GameManager();
		~GameManager();

		bool IsGameRunning();

		void InjectModule(const std::string& modulePath);

		void StartGame();
		void KillGame();

		// TODO: Send a message to the
		// supervisor to stop the game
		// void StopGame();

	private:
		// Checks every 1s if the game is running
		std::thread gameStatusThread;
		std::mutex gameStatusMutex;

		std::thread moduleHandlerThread;

		// Tracks when the game was first seen as running by the GameManager
		std::optional<std::chrono::time_point<std::chrono::system_clock>> gameStartedTime;

		std::vector<MODULEENTRY32> modules;
		bool gameRunning = false;

		DWORD pid = 0;
	};
}; // namespace Carbon
