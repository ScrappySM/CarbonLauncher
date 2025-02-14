#include "pch.h"

#include "managers/game.h"

using namespace CL;

GameManager::GameManager() {
	this->checkThread = std::thread([this]() {
		while (this->threadRunning) {
			std::this_thread::sleep_for(std::chrono::seconds(1));

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

GameManager::~GameManager() {
	this->threadRunning = false;

	if (this->checkThread.joinable())
		this->checkThread.join();
}

void GameManager::LaunchGame() {
	// steam appid 387990, steam://rungameid/387990
	std::thread([this]() {
		ShellExecuteA(NULL, "open", this->launchCommand, NULL, NULL, SW_SHOWNORMAL);
		}).detach();
}

void GameManager::KillGame() {
	std::thread([this]() {
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

bool GameManager::IsRunning() {
	std::lock_guard<std::mutex> lock(this->mtx);
	return this->gameRunning;
}
