#pragma once

#include "pch.h"

namespace Managers {
	class Game {
	public:
		Game();
		~Game();

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;

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

		// void SetTargetName(const std::string_view& name);

	private:
		bool threadRunning = false;
		bool gameRunning = false;

		const char* targetName = "ScrapMechanic.exe";
		const char* launchCommand = "steam://rungameid/387990";

		std::thread checkThread;
		std::mutex mtx;
	};
} // namespace Managers
