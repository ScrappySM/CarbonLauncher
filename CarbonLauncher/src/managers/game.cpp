#include "managers/game.h"

namespace Managers {
	Game::Game() {
		ZoneScoped;
		this->checkThread = std::thread([this]() {
			tracy::SetThreadName("Managers::Game");

			while (this->threadRunning) {
				std::this_thread::sleep_for(std::chrono::seconds(1));

				ZoneScopedN("checkGamerunning");

				HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (hSnap == INVALID_HANDLE_VALUE) {
					spdlog::error("Failed to create snapshot");
					continue;
				}

				PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
				if (!Process32First(hSnap, &pe32)) {
					spdlog::error("Failed to get first process");
					CloseHandle(hSnap);
					continue;
				}

				bool found = false;
				do {
					if (strcmp(pe32.szExeFile, this->targetName) == 0) {
						found = true;
						break;
					}
				} while (Process32Next(hSnap, &pe32));

				{
					std::lock_guard<std::mutex> lock(this->mtx);
					this->gameRunning = found;
				}
			}
			});
		this->threadRunning = true;
	}

	Game::~Game() {
		ZoneScoped;

		this->threadRunning = false;

		if (this->checkThread.joinable()) {
			this->checkThread.join();
		}
	}

	void Game::LaunchGame() {
		ZoneScoped;

		// steam appid 387990, steam://rungameid/387990
		std::thread([this]() {
			tracy::SetThreadName("Managers::Game::LaunchGame");

			ZoneScopedN("LaunchGame");
			ShellExecuteA(NULL, "open", this->launchCommand, NULL, NULL, SW_SHOWNORMAL);
			}).detach();
	}

	void Game::KillGame() {
		ZoneScoped;

		std::thread([this]() {
			tracy::SetThreadName("Managers::Game::KillGame");
			ZoneScopedN("KillGame");

			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnap == INVALID_HANDLE_VALUE) {
				spdlog::error("Failed to create snapshot");
				return;
			}

			PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
			if (!Process32First(hSnap, &pe32)) {
				spdlog::error("Failed to get first process");
				CloseHandle(hSnap);
				return;
			}

			do {
				if (strcmp(pe32.szExeFile, this->targetName) == 0) {
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
					if (hProcess == NULL) {
						spdlog::error("Failed to open process");
						CloseHandle(hSnap);
						return;
					}

					if (!TerminateProcess(hProcess, 0)) {
						spdlog::error("Failed to terminate process");
					}

					CloseHandle(hProcess);
					break;
				}
			} while (Process32Next(hSnap, &pe32));
			CloseHandle(hSnap);
			}).detach();
	}

	bool Game::IsRunning() {
		ZoneScoped;
		std::lock_guard<std::mutex> lock(this->mtx);
		return this->gameRunning;
	}

	/*void Game::SetTargetName(const std::string_view& name) {
		ZoneScoped;
		std::lock_guard<std::mutex> lock(this->mtx);
		this->targetName = name.data();
	}*/
} // namespace Managers