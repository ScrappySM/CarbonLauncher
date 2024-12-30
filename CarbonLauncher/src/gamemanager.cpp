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
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
				// This may occur if the game was started before Carbon Launcher
				// was opened.
				if (!this->gameStartedTime.has_value()) {
					this->gameStartedTime = std::chrono::system_clock::now();
				}

				std::lock_guard<std::mutex> lock(this->gameStatusMutex);

				if (found != this->gameRunning) {
					spdlog::info("Game is {}", found ? "running" : "not running");
				}

				this->gameRunning = true;
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

			// Allow the thread some breathing room
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});

	this->gameStatusThread.detach();

	this->moduleHandlerThread = std::thread([this]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			bool hasInjectedAnything = false;
			if (this->IsGameRunning()) {
				if (!C.pipeManager.GetPacketsByType(PacketType::LOADED).empty()) {
					// Get module list
					auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
					if (!hProcess) {
						spdlog::error("Failed to open process with PID {}", pid);
						continue;
					}

					MODULEENTRY32 moduleEntry{};
					moduleEntry.dwSize = sizeof(moduleEntry);

					auto hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
					if (hModuleSnap == INVALID_HANDLE_VALUE) {
						CloseHandle(hProcess);
						continue;
					}

					if (!Module32First(hModuleSnap, &moduleEntry)) {
						CloseHandle(hModuleSnap);
						CloseHandle(hProcess);
						continue;
					}

					do {
						this->modules.emplace_back(moduleEntry);
					} while (Module32Next(hModuleSnap, &moduleEntry));

					CloseHandle(hModuleSnap);
					CloseHandle(hProcess);

					this->modules = modules;

					spdlog::info("Game loaded, injecting modules");

					std::string modulesDir = Utils::GetCurrentModuleDir() + "mods";
					std::filesystem::create_directory(modulesDir);

					for (auto& module : std::filesystem::recursive_directory_iterator(modulesDir)) {
						if (!module.is_regular_file() || module.path().extension() != ".dll")
							continue; // Skip directories and non-DLL files

						bool found = false;
						for (auto& foundModule : this->modules) {
							if (std::wstring(module.path().filename().wstring()) == foundModule.szModule) {
								found = true;
								break;
							}
						}

						if (!found) {
							this->InjectModule(module.path().string());
							hasInjectedAnything = true;
						}
						else {
							spdlog::warn("Module `{}` is already loaded", module.path().string());
						}
					}

					if (hasInjectedAnything) {
						this->ModulesInjected = true;
					}
				}
			}
		}
	});

	this->moduleHandlerThread.detach();
}

GameManager::~GameManager() { }

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
	this->gameStartedTime = std::nullopt; 
	this->gameRunning = false;
	this->pid = 0;
	this->modules.clear();
	this->ModulesInjected = false;

	std::thread([&]() {
		if (C.processTarget == "ScrapMechanic.exe") {
			ShellExecute(NULL, L"open", L"steam://rungameid/387990", NULL, NULL, SW_SHOWNORMAL);
		}
		else {
			std::string exePath = Utils::GetCurrentModuleDir() + C.processTarget;
			ShellExecute(NULL, L"open", std::wstring(exePath.begin(), exePath.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}

		while (!this->IsGameRunning()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		this->gameStartedTime = std::chrono::system_clock::now();

		std::this_thread::sleep_for(std::chrono::seconds(1));
		spdlog::info("Game is running, injecting CarbonSupervisor.dll");
		this->InjectModule(Utils::GetCurrentModuleDir() + "CarbonSupervisor.dll");
		}).detach();
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

	while (this->IsGameRunning()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool GameManager::IsModuleLoaded(const std::string& moduleName) const {
	std::vector<MODULEENTRY32> modules;
	auto hProcesses = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, this->pid);
	if (hProcesses == INVALID_HANDLE_VALUE) {
		return false;
	}

	MODULEENTRY32 entry{ sizeof(entry) };
	
	if (!Module32First(hProcesses, &entry)) {
		CloseHandle(hProcesses);
		return false;
	}

	do {
		modules.emplace_back(entry);
	} while (Module32Next(hProcesses, &entry));

	CloseHandle(hProcesses);

	for (auto& module : modules) {
		std::wstring moduleNameW(module.szModule);
		std::string moduleNameA(moduleNameW.begin(), moduleNameW.end());
		if (moduleNameA == moduleName) {
			return true;
		}
	}

	return false;
}

int GameManager::GetLoadedCustomModules() {
	// Wait for injection to happen
	while (!this->ModulesInjected) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	int count = 0;

	for (auto& file : std::filesystem::recursive_directory_iterator(Utils::GetCurrentModuleDir() + "mods")) {
		if (file.is_regular_file() && file.path().extension() == ".dll") {
			if (this->IsModuleLoaded(file.path().filename().string())) {
				count++;
			}
		}
	}

	return count;
}
