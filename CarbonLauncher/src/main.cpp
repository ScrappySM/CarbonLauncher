#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "dwmapi.lib")

#include <Windows.h>
#include <Tlhelp32.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#undef CreateWindow
#include "utils.h"

// Discord game SDK
#include <discord-game-sdk/discord_game_sdk.h>
#include <discord-game-sdk/discord.h>

struct State_t {
	std::vector<std::filesystem::path> enabledMods;

struct DiscordState_t {
    discord::User currentUser;

    std::unique_ptr<discord::Core> core;
} DiscordState;
};

State_t State;

constexpr auto discordClientId = 1315436867545595904;


int main(int argc, char* argv[]) {
	// Check for other processes (CarbonLauncher.exe)
	auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) {
		spdlog::error("Failed to create snapshot");
		return -1;
	}

	PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
	if (!Process32First(snap, &pe32)) {
		spdlog::error("Failed to get first process");
		return -1;
	}

	do {
		if (strcmp(pe32.szExeFile, "CarbonLauncher.exe") == 0) {
			static int count = 0;
			count++;

			if (count > 1) {
				spdlog::error("Another instance of Carbon Launcher is already running");
				return -1;
			}
		}
	} while (Process32Next(snap, &pe32));

	State.DiscordState = {};

    discord::Core* core{};
	auto result = discord::Core::Create(discordClientId, DiscordCreateFlags_NoRequireDiscord, &core);
    State.DiscordState.core.reset(core);
	if (!State.DiscordState.core) {
		spdlog::error("Failed to create Discord core");
        std::exit(-1);
    }

	auto dCore = State.DiscordState.core.get();

	dCore->SetLogHook(discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
		spdlog::debug("[Discord] {}", message);
	});

	dCore->SetLogHook(discord::LogLevel::Info, [](discord::LogLevel level, const char* message) {
		spdlog::info("[Discord] {}", message);
		});

	dCore->SetLogHook(discord::LogLevel::Warn, [](discord::LogLevel level, const char* message) {
		spdlog::warn("[Discord] {}", message);
		});

	dCore->SetLogHook(discord::LogLevel::Error, [](discord::LogLevel level, const char* message) {
		spdlog::error("[Discord] {}", message);
		});

	State.DiscordState.core->ActivityManager().RegisterCommand("carbonlauncher://run");

	// Show Carbon Launcher as RPC
	discord::Activity activity{};
	activity.SetDetails("In the launcher");
	activity.SetState("The latest SM modded launcher");
	activity.GetAssets().SetLargeText("Carbon Launcher");
	activity.GetAssets().SetSmallImage("carbon_launcher");
	activity.SetType(discord::ActivityType::Playing);

	activity.SetSupportedPlatforms((uint32_t)discord::ActivitySupportedPlatformFlags::Desktop);

	State.DiscordState.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		if (result == discord::Result::Ok) {
			spdlog::info("Discord RPC updated");
		}
		else {
			spdlog::error("Failed to update Discord RPC");
		}
		});

	GLFWwindow* window = CreateWindow(1280, 720, "Carbon Launcher");
	InitImGui(window);

	HWND hWnd = glfwGetWin32Window(window);
	BOOL compositionEnabled = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &compositionEnabled, sizeof(BOOL));

	// Create the `mods` directory if it doesn't exist
	std::string modsDir = GetExeDirectory() + "\\mods";
	std::filesystem::create_directory(modsDir);

	// Create the `logs` directory if it doesn't exist
	std::string logsDir = GetExeDirectory() + "\\logs";
	std::filesystem::create_directory(logsDir);

	// Create the `settings` directory if it doesn't exist
	std::string settingsDir = GetExeDirectory() + "\\settings";
	std::filesystem::create_directory(settingsDir);

	ImGui::GetIO().IniFilename = nullptr;

	// Load from settings/enabled.txt
	auto loadEnabledMods = [&]() {
		std::ifstream file(settingsDir + "\\enabled.txt");
		if (file.is_open()) {
			std::string line;
			while (std::getline(file, line)) {
				State.enabledMods.push_back(line);
			}
		}
		};

	auto saveEnabledMods = [&]() {
		std::ofstream file(settingsDir + "\\enabled.txt");
		if (file.is_open()) {
			for (const auto& mod : State.enabledMods) {
				file << mod.string() << std::endl;
			}
		}
		};

	loadEnabledMods();

	Render(window, [&]() {
		dCore->RunCallbacks();

		int w, h = 0;
		glfwGetWindowSize(window, &w, &h);

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
		bool popen = true;
		ImGui::Begin("Carbon Launcher", &popen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);

		bool shouldOpenPopup = false;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Exit"))
					glfwSetWindowShouldClose(window, GLFW_TRUE);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem("About")) {
					shouldOpenPopup = true;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		static std::chrono::time_point<std::chrono::system_clock> popupOpenTime;

		if (shouldOpenPopup) {
			ImGui::OpenPopup("Info");
			popupOpenTime = std::chrono::system_clock::now();
		}

		bool isInfoPopupOpen = true;
		if (ImGui::BeginPopupModal("Info", &isInfoPopupOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Carbon Launcher");
			ImGui::Separator();
			ImGui::Text("Version 0.1.0");
			ImGui::Text("By Ben McAvoy");

			if (!isInfoPopupOpen)
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		// Create a childwindow with three tabs, "Mods" and "Settings" and "Logs".
		if (ImGui::BeginTabBar("Tabs")) {
			if (ImGui::BeginTabItem("Mods")) {
				auto dirIter = std::filesystem::directory_iterator(modsDir);
				for (const auto& entry : dirIter) {
					ImGui::BeginChild(entry.path().string().c_str(), ImVec2(0, 200), true);
					ImGui::TextWrapped(entry.path().filename().string().c_str());

					bool isModEnabled = std::find(State.enabledMods.begin(), State.enabledMods.end(), entry.path()) != State.enabledMods.end();
					if (ImGui::Checkbox("Enabled", &isModEnabled)) {
						if (isModEnabled) {
							State.enabledMods.push_back(entry.path());
						}
						else {
							State.enabledMods.erase(std::remove(State.enabledMods.begin(), State.enabledMods.end(), entry.path()), State.enabledMods.end());
						}

						saveEnabledMods();
					}

					ImGui::EndChild();
				}

				if (dirIter == std::filesystem::directory_iterator())
					ImGui::Text("No mods found :(");

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings")) {
				ImGui::BeginChild("Directories", ImVec2(0, 0), false);
				if (ImGui::TreeNode("Directories")) {
					if (ImGui::Button("Open Mods Directory")) {
						ShellExecuteA(NULL, "open", modsDir.c_str(), NULL, NULL, SW_SHOWNORMAL);
					}

					if (ImGui::Button("Open Logs Directory")) {
						ShellExecuteA(NULL, "open", logsDir.c_str(), NULL, NULL, SW_SHOWNORMAL);
					}

					if (ImGui::Button("Open Settings Directory")) {
						ShellExecuteA(NULL, "open", settingsDir.c_str(), NULL, NULL, SW_SHOWNORMAL);
					}

					ImGui::TreePop();
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Logs")) {
				ImGui::Text("Logs");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		static bool oldIsGameRunning = false;
		static bool isGameRunning = false;
		static std::chrono::time_point timeSinceCheckedOpen = std::chrono::system_clock::now() - std::chrono::seconds(2);
		if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - timeSinceCheckedOpen).count() > 1) {
			timeSinceCheckedOpen = std::chrono::system_clock::now();
			isGameRunning = IsGameOpen();
		}

		// If isGameRunning changes, update the activity
		if (oldIsGameRunning != isGameRunning) {
			activity.SetDetails(isGameRunning ? "Playing Scrap Mechanic" : "In the launcher");
			std::string modCount = fmt::format("{} mods enabled", State.enabledMods.size());
			activity.SetState(isGameRunning ? modCount.c_str() : "The latest SM modded launcher");

			State.DiscordState.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
				if (result == discord::Result::Ok) {
					spdlog::info("Discord RPC updated");
				}
				else {
					spdlog::error("Failed to update Discord RPC");
				}
				});

			oldIsGameRunning = isGameRunning;
		}

		static float buttonSize = ImGui::CalcItemWidth();
		ImVec2 size = ImVec2(buttonSize, 20);
		const char* text = isGameRunning ? "Stop" : "Start";
		ImVec2 pos = ImVec2((w - size.x) / 2, size.y);
		ImGui::SetCursorPosX(pos.x);
		ImGui::SetCursorPosY(h - pos.y - 24);
		if (ImGui::Button(text, ImVec2(size.x + 20, size.y + 10))) {
			if (isGameRunning) {
				ShellExecuteA(NULL, "open", "taskkill", "/F /IM ScrapMechanic.exe", NULL, SW_SHOWNORMAL);
			}
			else {
				ShellExecuteA(NULL, "open", "steam://run/387990/-dev", NULL, NULL, SW_SHOWMINIMIZED);

				// Management of injection after message from CarbonSupervisor.
				std::thread([&] {
					// Inject CarbonSupervisor.dll
					DWORD targetPID = GetProcID("ScrapMechanic.exe");
					/*if (targetPID == 0) {
						spdlog::error("Failed to get process ID of ScrapMechanic.exe");
						return 1;
					}*/

					while (targetPID == 0) {
						spdlog::info("Waiting for ScrapMechanic.exe to start");
						std::this_thread::sleep_for(std::chrono::seconds(2));
						targetPID = GetProcID("ScrapMechanic.exe");
					}

					std::string supervisorPath = GetExeDirectory() + "\\CarbonSupervisor.dll";
					if (!Inject(targetPID, hWnd, supervisorPath)) {
						spdlog::error("Failed to inject CarbonSupervisor.dll");
						return 1;
					}

					// Allow some initialization time
					std::this_thread::sleep_for(std::chrono::seconds(1));

					// *Connect* to the supervisor pipe
					HANDLE hPipe = CreateFileA("\\\\.\\pipe\\CarbonSupervisor", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
					if (hPipe == INVALID_HANDLE_VALUE) {
						spdlog::error("Failed to open named pipe: {}", GetLastErrorAsString());
						return 1;
					}

					spdlog::info("Opened named pipe");

					// Wait for the supervisor to send `loaded`
					char buffer[7] = { 0 };
					DWORD bytesRead;
					if (!ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr)) {
						spdlog::error("Failed to read from named pipe: {}", GetLastErrorAsString());
						return 1;
					}

					buffer[bytesRead - 1] = '\0';  // Null-terminate after each read.

					spdlog::info("Received message: {}", buffer);

					
					while (strcmp(buffer, "loaded") != 0) {
						// Clear the buffer before reading again
						memset(buffer, 0, sizeof(buffer));

						if (!ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr)) {
							spdlog::error("Failed to read from named pipe: {}", GetLastErrorAsString());
							return 1;
						}

						buffer[bytesRead] = '\0';  // Null-terminate after each read.

						spdlog::info("Received message: {}", buffer);
					}

					// Inject mods
					std::vector enabledModsClone = State.enabledMods;
					for (auto& mod : enabledModsClone) {
						if (!Inject(targetPID, hWnd, mod.string())) {
							spdlog::error("Failed to inject mod: {}", mod.string());
							continue;
						}
					}

					return 0;
				}).detach();
			}
		}

		// Drop up menu for the kill button, should only be shown when the game is running
		if (isGameRunning) {
			ImGui::SetCursorPosX(pos.x + size.x + 24);
			ImGui::SetCursorPosY(h - pos.y - 24);

			bool isKillPopupOpen = false;
			if (ImGui::Button("^", ImVec2(24, size.y + 10))) {
				isKillPopupOpen = true;
			}

			if (isKillPopupOpen)
				ImGui::OpenPopup("KillGame");

			if (ImGui::BeginPopupModal("KillGame", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Are you sure you want to kill the game?");
				ImGui::Separator();

				if (ImGui::Button("Yes")) {
					ShellExecuteA(NULL, "open", "taskkill", "/F /IM ScrapMechanic.exe", NULL, SW_SHOWMINIMIZED);
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("No")) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::End();

		if (!popen)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}