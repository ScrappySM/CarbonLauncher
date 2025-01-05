#pragma comment(lib, "dwmapi.lib")
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include "resource.h"

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
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <font/IconsFontAwesome6.h>
#include <font/IconsFontAwesome6.h_fa-solid-900.ttf.h>

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
	style.FramePadding = ImVec2(8, 4);
	style.TabBarBorderSize = 2;
	style.ScrollbarSize = 10;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 12.0f;
	style.TabBarBorderSize = 0.0f;
	style.ChildRounding = 6.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 12.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.PopupBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;

	constexpr float fontSize = 18.0f;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", fontSize);

	constexpr float iconFontSize = fontSize * 2.0f / 3.0f;
	void* data = (void*)s_fa_solid_900_ttf;
	int size = sizeof(s_fa_solid_900_ttf);

	static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig iconsConfig;
	iconsConfig.MergeMode = true;
	iconsConfig.PixelSnapH = true;
	iconsConfig.GlyphMinAdvanceX = iconFontSize;
	iconsConfig.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF(data, size, iconFontSize, &iconsConfig, iconsRanges);
	io.Fonts->Build();

	// Go through every colour and get hsv values, if it's 151 then change it to 0 and then set the colour
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		ImVec4* col = &style.Colors[i];
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(col->x, col->y, col->z, h, s, v);
		if (h > 0.59 && h < 0.61) {
			h = 0.0f;
			s = 0.5f;
			spdlog::info("Modified colour {}", i);
		}
		else {
			spdlog::info("Colour {} is {}", i, h);
		}
		ImGui::ColorConvertHSVtoRGB(h, s, v, col->x, col->y, col->z);
	}

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

	bool shouldOpenPopup = false;
	// Begin main menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				glfwSetWindowShouldClose(C.guiManager.window, GLFW_TRUE);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) {
				//ImGui::OpenPopup("About Carbon Launcher");
				shouldOpenPopup = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (shouldOpenPopup) {
		ImGui::OpenPopup("About Carbon Launcher");
	}

	if (ImGui::BeginPopupModal("About Carbon Launcher", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Carbon Launcher");
		ImGui::Text("Version 1.0.0");
		ImGui::Text("Developed by @BenMcAvoy");
		if (ImGui::Button("View their GitHub")) {
			ShellExecute(NULL, L"open", L"https://github.com/BenMcAvoy", NULL, NULL, SW_SHOWNORMAL);
		}

		ImGui::SameLine();

		if (ImGui::Button("Close")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	int w, h;
	glfwGetWindowSize(C.guiManager.window, &w, &h);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
	ImGui::Begin("Carbon Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_MenuBar);

	ImGui::BeginChild("Control", ImVec2(0, 64), true);

	if (C.gameManager.IsGameRunning()) {
		if (ImGui::Button(ICON_FA_STOP " Kill Game", ImVec2(128, 48))) {
			C.gameManager.KillGame();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Kill the game and all its processes, should be used as a last resort!");
			ImGui::EndTooltip();
		}
	}
	else {
		if (ImGui::Button(ICON_FA_PLAY " Launch Game", ImVec2(128, 48))) {
			C.gameManager.LaunchGame();
		}
	}

	ImGui::SameLine();

	// Display if the pipe is connected or not if the game is running
	if (C.gameManager.IsGameRunning()) {
		if (!C.pipeManager.IsConnected()) {
			static ImVec4 colours[3] = {};
			if (colours[0].x == 0) {
				colours[0] = ImGui::GetStyleColorVec4(ImGuiCol_Button);
				colours[1] = ImGui::GetStyleColorVec4(ImGuiCol_Button);
				colours[2] = ImGui::GetStyleColorVec4(ImGuiCol_Button);

				for (int i = 0; i < 3; i++) {
					colours[i].w += 0.5f;

					float h, s, v;
					ImGui::ColorConvertRGBtoHSV(colours[i].x, colours[i].y, colours[i].z, h, s, v);
					h = 0.0f;
					s = 0.9f;
					ImGui::ColorConvertHSVtoRGB(h, s, v, colours[i].x, colours[i].y, colours[i].z);
				}
			}

			ImGui::PushStyleColor(ImGuiCol_Button, colours[0]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colours[1]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colours[2]);

			ImGui::Button(ICON_FA_X " Pipe Disconnected! " ICON_FA_FACE_SAD_TEAR, ImVec2(256, 48));

			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("The pipe is disconnected, the game may not be running correctly and Carbon Launcher cannot communicate with it.");
				ImGui::EndTooltip();
			}

			ImGui::PopStyleColor(3);
		}
	}

	ImGui::EndChild();

	ImGui::BeginChild("Tabs", ImVec2(64, 0), true);

	auto highlight = [&](GUIManager::Tab tab) -> bool {
		static ImVec4 colours[3] = {};
		if (colours[0].x == 0) {
			colours[0] = ImGui::GetStyleColorVec4(ImGuiCol_Button);
			colours[1] = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
			colours[2] = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

			colours[0].w += 0.5f;
			colours[1].w += 0.5f;
			colours[2].w += 0.5f;
		}
		if (C.guiManager.tab == tab) {
			ImGui::PushStyleColor(ImGuiCol_Button, colours[0]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colours[1]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colours[1]);
		}

		return C.guiManager.tab == tab;
		};

	auto renderTag = [&](GUIManager::Tab tag, const char* icon, const char* tooltip) -> void {
		bool highlighted = highlight(tag);

		if (ImGui::Button(icon, ImVec2(48, 48))) {
			C.guiManager.tab = tag;
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text(tooltip);
			ImGui::EndTooltip();
		}

		if (highlighted)
			ImGui::PopStyleColor(3);
		};

	renderTag(GUIManager::Tab::MyMods, ICON_FA_PUZZLE_PIECE, "My Mods, where you can manage the mods you have installed");
	renderTag(GUIManager::Tab::Discover, ICON_FA_SHOP, "Discover, where you can discover new mods");
	renderTag(GUIManager::Tab::Console, ICON_FA_TERMINAL, "Console, where you can see the output of the game");
	renderTag(GUIManager::Tab::Settings, ICON_FA_GEAR, "Settings, where you can configure the launcher");

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Content", ImVec2(0, 0), true);

	auto renderMod = [&](Mod& mod) -> void {
		ImGui::BeginChild(mod.ghRepo.c_str(), ImVec2(0, 72), false);

		float buttons = mod.wantsUpdate ? 2.75 : 2;
		ImGui::BeginChild("Details", ImVec2(ImGui::GetContentRegionAvail().x - (64 * buttons), 0), false);

		ImGui::SetWindowFontScale(1.2f);
		ImGui::TextWrapped(mod.name.c_str());
		ImGui::SetWindowFontScale(1.0f);

		ImGui::TextWrapped(mod.description.c_str());

		ImGui::TextWrapped("Authors:");
		ImGui::SameLine();
		auto authBegin = mod.authors.begin();
		auto authEnd = mod.authors.end();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
		for (auto it = authBegin; it != authEnd; it++) {
			auto& author = *it;
			if (it != authBegin) {
				ImGui::SameLine();
				ImGui::TextWrapped(", ");
			}
			ImGui::SameLine();
			if (ImGui::SmallButton(author.c_str())) {
				std::string url = fmt::format("https://github.com/{}", author);
				ShellExecute(NULL, L"open", std::wstring(url.begin(), url.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}
		ImGui::PopStyleVar();

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Management", ImVec2(0, 0), false);

		if (mod.wantsUpdate) {
			if (ImGui::Button(ICON_FA_ARROWS_SPIN, ImVec2(48, 48))) {
				mod.Update();
			}

			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Update the mod");
				ImGui::EndTooltip();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_GLOBE, ImVec2(48, 48))) {
			std::string url = fmt::format("https://github.com/{}/{}", mod.ghUser, mod.ghRepo);
			ShellExecute(NULL, L"open", std::wstring(url.begin(), url.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Open the mods GitHub page");
			ImGui::EndTooltip();
		}

		ImGui::SameLine();

		if (mod.installed) {
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(48, 48))) {
				mod.Uninstall();
			}
		}
		else {
			if (ImGui::Button(ICON_FA_DOWNLOAD, ImVec2(48, 48))) {
				mod.Install();
			}
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text(mod.installed ? "Uninstall the mod" : "Install the mod");
			ImGui::EndTooltip();
		}

		ImGui::EndChild();
		ImGui::EndChild();
		};

	auto renderMods = [&](bool mustBeInstalled) -> void {
		for (auto& mod : C.repoManager.GetMods()) {
			if (mod.installed == mustBeInstalled) {
				renderMod(mod);
			}
		}

		if (std::all_of(C.repoManager.GetMods().begin(), C.repoManager.GetMods().end(), [&](Mod& mod) -> bool {
			return mod.installed != mustBeInstalled;
			})) {
			if (mustBeInstalled) {
				ImGui::TextWrapped("No mods installed! Get some from the discover tab " ICON_FA_FACE_SMILE);
				if (ImGui::Button("Take me there!")) {
					C.guiManager.tab = GUIManager::Tab::Discover;
				}
			}
			else {
				ImGui::TextWrapped("No more mods to discover! Check back later " ICON_FA_FACE_SMILE);
			}
		}
		};

	auto renderConsole = [&]() -> void {
		ImGui::BeginChild("Console", ImVec2(0, 0), true);
		auto begin = C.logMessages.begin();
		auto end = C.logMessages.end();
		for (auto it = begin; it != end; it++) {
			auto& message = *it;
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), message.time.c_str());
			ImGui::SameLine();

			static auto typeToColour = std::map<LogColour, ImVec4>{
				{ LogColour::DARKGREEN, ImVec4(0.2f, 0.6f, 0.2f, 1.0f) },
				{ LogColour::BLUE, ImVec4(0.2f, 0.2f, 0.6f, 1.0f) },
				{ LogColour::PURPLE, ImVec4(0.5f, 0.2f, 0.5f, 1.0f) },
				{ LogColour::GOLD, ImVec4(0.7f, 0.5f, 0.2f, 1.0f) },
				{ LogColour::WHITE, ImVec4(0.8f, 0.8f, 0.8f, 1.0f) },
				{ LogColour::DARKGRAY, ImVec4(0.3f, 0.3f, 0.3f, 1.0f) },
				{ LogColour::DARKBLUE, ImVec4(0.1f, 0.1f, 0.4f, 1.0f) },
				{ LogColour::GREEN, ImVec4(0.2f, 0.7f, 0.2f, 1.0f) },
				{ LogColour::CYAN, ImVec4(0.2f, 0.7f, 0.7f, 1.0f) },
				{ LogColour::RED, ImVec4(0.7f, 0.2f, 0.2f, 1.0f) },
				{ LogColour::PINK, ImVec4(0.7f, 0.2f, 0.7f, 1.0f) },
				{ LogColour::YELLOW, ImVec4(0.7f, 0.7f, 0.2f, 1.0f) },
			};

			ImGui::PushStyleColor(ImGuiCol_Text, typeToColour[static_cast<LogColour>(message.colour)]);
			ImGui::TextWrapped(message.message.c_str());
			ImGui::PopStyleColor();
		}

		// If we were scrolled to the bottom, scroll down
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
		};

	switch (C.guiManager.tab) {
	case GUIManager::Tab::MyMods:
		ImGui::SeparatorText("My Mods");
		renderMods(true);
		break;
	case GUIManager::Tab::Discover:
		ImGui::SeparatorText("Discover");
		renderMods(false);
		break;
	case GUIManager::Tab::Console:
		ImGui::SeparatorText("Console");
		renderConsole();

		break;
	case GUIManager::Tab::Settings:
		ImGui::SeparatorText("Settings");

		if (ImGui::Button("Open Mods Directory")) {
			std::string path = Utils::GetDataDir() + "mods";
			ShellExecute(NULL, L"open", std::wstring(path.begin(), path.end()).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}

		break;
	};

	ImGui::EndChild();

	ImGui::End();
}
