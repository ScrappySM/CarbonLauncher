#include "gamemanager.h"
#include "state.h"
#include "utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <TlHelp32.h>
#include <shellapi.h>

#include <spdlog/spdlog.h>

#include <filesystem>

using namespace Carbon;

GameManager::GameManager() {
	this->gameStatusThread = std::thread([this]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(1));

			auto hProcesses = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hProcesses == INVALID_HANDLE_VALUE) {
				continue;
			}

			PROCESSENTRY32 entry{};
			entry.dwSize = sizeof(entry);

			if (!Process32First(hProcesses, &entry)) {
				CloseHandle(hProcesses);
				continue;
			}

			bool found = false;
			do {
				std::string target(C.processTarget);
				if (std::wstring(entry.szExeFile) == std::wstring(target.begin(), target.end())) {
					found = true;
					break;
				}
			} while (Process32Next(hProcesses, &entry));

			CloseHandle(hProcesses);

			if (found) {
				auto hModules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, entry.th32ProcessID);
				if (hModules == INVALID_HANDLE_VALUE) {
					continue;
				}

				MODULEENTRY32 module{};
				module.dwSize = sizeof(module);

				if (!Module32First(hModules, &module)) {
					CloseHandle(hModules);
					continue;
				}

				std::vector<MODULEENTRY32> modules;
				do {
					modules.push_back(module);
				} while (Module32Next(hModules, &module));

				CloseHandle(hModules);

				std::lock_guard<std::mutex> lock(this->gameStatusMutex);
				this->gameRunning = true;
				this->modules = modules;
				this->pid = entry.th32ProcessID;

				if (!this->gameStartedTime.has_value()) {
					this->gameStartedTime = std::chrono::system_clock::now();
				}
			}
			else {
				std::lock_guard<std::mutex> lock(this->gameStatusMutex);
				this->gameRunning = false;
				this->modules.clear();
				this->gameStartedTime = std::nullopt;
				this->pid = 0;
			}
		}
		});
}

GameManager::~GameManager() {
	this->gameStatusThread.join();
}

bool GameManager::IsGameRunning() {
	std::lock_guard<std::mutex> lock(this->gameStatusMutex);
	return this->gameRunning;
}

void GameManager::InjectModule(const std::string& modulePath) {
	if (!this->IsGameRunning() || this->pid == 0) {
		spdlog::warn("Game manager was requested to inject `{}`, but the game is not running", modulePath);
		return;
	}

	if (!std::filesystem::exists(modulePath)) {
		spdlog::error("Module `{}` does not exist", modulePath);
		return;
	}

	auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess) {
		spdlog::error("Failed to open process with PID {}", pid);
		return;
	}

	auto modulePathW = std::wstring(modulePath.begin(), modulePath.end());
	auto modulePathSize = (modulePath.size() + 1) * sizeof(wchar_t);
	auto modulePathRemote = VirtualAllocEx(hProcess, NULL, modulePathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	WriteProcessMemory(hProcess, modulePathRemote, modulePathW.c_str(), modulePathSize, NULL);
	auto hKernel32 = GetModuleHandle(L"Kernel32.dll");
	auto hLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
	auto hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)hLoadLibrary, modulePathRemote, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	spdlog::info("Injected module `{}`", modulePath);
}

void GameManager::StartGame() {
	if (C.processTarget == "ScrapMechanic.exe") {
		ShellExecute(NULL, L"open", L"steam://rungameid/387990", NULL, NULL, SW_SHOWNORMAL);
		return;
	}

	std::string exePath = Utils::GetCurrentModuleDir() + C.processTarget;
	ShellExecute(NULL, L"open", std::wstring(exePath.begin(), exePath.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void GameManager::KillGame() {
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return;
	}
	PROCESSENTRY32 entry{};
	entry.dwSize = sizeof(entry);
	if (!Process32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return;
	}
	do {
		std::string target(C.processTarget);
		if (std::wstring(entry.szExeFile) == std::wstring(target.begin(), target.end())) {
			HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
			if (process) {
				TerminateProcess(process, 0);
				CloseHandle(process);
			}
		}
	} while (Process32Next(snapshot, &entry));
	CloseHandle(snapshot);
}
