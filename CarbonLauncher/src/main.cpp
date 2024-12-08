#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "dwmapi.lib")

#include <Windows.h>
#include <Tlhelp32.h>
#include <dwmapi.h>
#include <shellapi.h>

#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#undef CreateWindow
#include "utils.h"

int main(int argc, char* argv[]) {
	GLFWwindow* window = CreateWindow(800, 600, "Carbon Launcher");
	InitImGui(window);

	HWND hWnd = glfwGetWin32Window(window);
	BOOL compositionEnabled = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &compositionEnabled, sizeof(BOOL));

	Render(window, [&]() {
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

		static bool isGameRunning = false;
		static std::chrono::time_point timeSinceCheckedOpen = std::chrono::system_clock::now() - std::chrono::seconds(2);
		if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - timeSinceCheckedOpen).count() > 1) {
			timeSinceCheckedOpen = std::chrono::system_clock::now();
			isGameRunning = IsGameOpen();
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