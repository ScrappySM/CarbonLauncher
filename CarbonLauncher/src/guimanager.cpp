#pragma comment(lib, "dwmapi.lib")
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include "guimanager.h"
#include "utils.h"
#include "state.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <dwmapi.h>

#include <cpr/cpr.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace Carbon;

GUIManager::GUIManager() : renderCallback(nullptr), window(nullptr) {
    // Initialize GLFW
    if (!glfwInit()) {
        MessageBox(NULL, L"GLFW Initialization Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    glfwSetErrorCallback([](int error, const char* description) {
        spdlog::error("GLFW Error {}: {}", error, description);
    });

    // Create the OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the GLFW window
    this->window = glfwCreateWindow(1280, 720, "Carbon Launcher", NULL, NULL);
    if (!this->window) {
        spdlog::error("GLFW Window Creation Failed!");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(this->window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("GLAD Initialization Failed!");
        glfwTerminate();
        return;
    }

    // Enable dark mode (Windows only)
    BOOL darkMode = TRUE;
    HWND hWnd = glfwGetWin32Window(this->window);
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	// Round everything, disable some frames
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.ChildRounding = 4.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.PopupBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", 18.0f);

	ImGui_ImplGlfw_InitForOpenGL(this->window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	glfwSwapInterval(1);
}

GUIManager::~GUIManager() {
	// Terminate GLFW
	glfwTerminate();

	// Terminate ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GUIManager::Run() const {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (this->renderCallback) {
			this->renderCallback();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

		// Slow us down if we're running too fast
		if (ImGui::GetIO().DeltaTime < 1.0f / 60.0f) {
			std::this_thread::sleep_for(std::chrono::milliseconds((int)((1.0f / 60.0f - ImGui::GetIO().DeltaTime) * 1000)));
		}
    }
}

void _GUI() {
	using namespace Carbon;

	C.discordManager.Update();

	// Begin main menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				glfwSetWindowShouldClose(C.guiManager.window, GLFW_TRUE);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	int w, h;
	glfwGetWindowSize(C.guiManager.window, &w, &h);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
	ImGui::Begin("Carbon Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_MenuBar);

    // Begin tabs
	if (ImGui::BeginTabBar("CarbonTabs", ImGuiTabBarFlags_None)) {
		// Begin the first tab
		if (ImGui::BeginTabItem("Home")) {
			if (!C.repoManager.hasLoaded) {
				ImGui::TextWrapped("Loading mods...");
				ImGui::EndTabItem();
				ImGui::End();
				return;
			}

			// Show each installed mod in a child window that spans the entire width of the window
			for (auto& mod : C.repoManager.GetMods()) {
				if (mod.installed) {
					ImGui::BeginChild(mod.name.c_str(), ImVec2(0, 150), true);
					ImGui::TextWrapped(mod.name.c_str());
					ImGui::Separator();
					ImGui::TextWrapped(mod.description.c_str());
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

					std::string authText = mod.authors.size() > 1 ? "Authors: " : "Author: ";
					ImGui::TextWrapped(authText.c_str());
					ImGui::SameLine();
					for (auto& author : mod.authors) {
						std::string link = fmt::format("https://github.com/{}", author);
						if (ImGui::Button(fmt::format("@{} ", author).c_str())) {
							ShellExecute(NULL, L"open", std::wstring(link.begin(), link.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
						}

						if (author != mod.authors.back()) {
							ImGui::SameLine();
						}
					}

					// Go to bottom of child window
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y);

					int frameWidth = (int)ImGui::GetContentRegionAvail().x;
					if (ImGui::Button("Uninstall", ImVec2((float)frameWidth, 0))) {
						if (C.gameManager.IsGameRunning()) {
							spdlog::error("TODO: Unload the mod from the game (ctx: tried to uninstall mod while game was running)");
							return;
						}

						mod.Uninstall();
					}

					if (mod.hasUpdate && ImGui::Button("Update")) {
						mod.Install();
					}
					else if (!mod.hasUpdate) {
						ImGui::Text("No updates");
					}

					ImGui::EndChild();
				}
			}

			if (std::all_of(C.repoManager.GetMods().begin(), C.repoManager.GetMods().end(), [](const Mod& mod) { return !mod.installed; })) {
				ImGui::TextWrapped("No mods installed :(");
				ImGui::TextWrapped("Check out the public mods tab to get started!");
			}

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 32);

			// Big centered start/kill game button
			int width = (int)(ImGui::GetWindowWidth() * 2 / 3); // 2/3 of the window width
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) / 2);
			if (ImGui::Button(C.gameManager.IsGameRunning() ? "Kill Game" : "Start Game", ImVec2((float)width, 40))) {
				if (C.gameManager.IsGameRunning()) {
					C.gameManager.KillGame();
				}
				else {
					C.gameManager.StartGame();
					std::this_thread::sleep_for(std::chrono::seconds(1));
					C.gameManager.InjectModule(Utils::GetCurrentModuleDir() + "CarbonSupervisor.dll");
				}
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Public mods")) {
			for (auto& mod : C.repoManager.GetMods()) {
				ImGui::BeginChild(mod.name.c_str(), ImVec2(0, 300), true);

				ImGui::TextWrapped(mod.name.c_str());
				ImGui::Separator();
				ImGui::TextWrapped(mod.description.c_str());

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);

				if (mod.authors.size() > 1) {
					ImGui::TextWrapped("Authors: ");
					ImGui::SameLine();

					for (auto& author : mod.authors) {
						std::string link = "https://github.com/" + author;
						std::string text = "@" + author + " ";

						if (ImGui::Button(text.c_str())) {
							ShellExecute(NULL, L"open", std::wstring(link.begin(), link.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
						}

						ImGui::PopStyleColor();

						if (author != mod.authors.back()) {
							ImGui::SameLine();
						}
					}
				}
				else {
					ImGui::TextWrapped("Author: ");
					ImGui::SameLine();

					std::string link = "https://github.com/" + mod.authors[0];
					std::string text = "@" + mod.authors[0] + " ";

					if (ImGui::Button(text.c_str())) {
						ShellExecute(NULL, L"open", std::wstring(link.begin(), link.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
					}
				}

				// Set ImGui cursor y pos to bottom of child window
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y);

				if (ImGui::Button(mod.installed ? "Uninstall" : "Install")) {
					if (mod.installed) {
						mod.Uninstall();
					}
					else {
						mod.Install();
					}
				}

				ImGui::EndChild();

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			ImGui::EndTabItem();
		}

		// Begin the second tab
		if (ImGui::BeginTabItem("Settings")) {
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}
