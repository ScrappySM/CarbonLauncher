#pragma once

#include "pch.h"

namespace CL {
	class GameManager {
	public:
		static GameManager& GetInstance() {
			static GameManager instance;
			return instance;
		}

		/// <summary>
		/// Launch the game
		/// </summary>
		void LaunchGame();

		/// <summary>
		/// Kill the game
		/// </summary>
		void KillGame();

		/// <summary>
		/// Check if the game was last seen running
		/// </summary>
		/// <returns>Whether the game was last seen running</returns>
		bool IsRunning();

	private:
		bool threadRunning = false;
		bool gameRunning = false;

		const char* targetName = "ScrapMechanic.exe";
		const char* launchCommand = "steam://rungameid/387990";

		std::thread checkThread;
		std::mutex mtx;

		GameManager();
		~GameManager();

		GameManager(const GameManager&) = delete;
		GameManager& operator=(const GameManager&) = delete;
	};
} // namespace CL
