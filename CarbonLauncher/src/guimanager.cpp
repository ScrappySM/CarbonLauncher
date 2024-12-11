#pragma comment(lib, "dwmapi.lib")
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN

#include "guimanager.h"
#include "utils.h"
#include "state.h"

#include <Windows.h>

#include <dwmapi.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace Carbon;

GUIManager::GUIManager(HINSTANCE hInstance) {
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
	(void)io;
	ImGui::StyleColorsDark();

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
    }
}

void _GUI() {
	using namespace Carbon;

	C.discordManager->Update();

	// Begin main menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				glfwSetWindowShouldClose(C.guiManager->window, GLFW_TRUE);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	int w, h;
	glfwGetWindowSize(C.guiManager->window, &w, &h);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
	ImGui::Begin("Carbon Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_MenuBar);

    // Begin tabs
	if (ImGui::BeginTabBar("CarbonTabs", ImGuiTabBarFlags_None)) {
		// Begin the first tab
		if (ImGui::BeginTabItem("Home")) {
			if (C.gameManager->IsGameRunning()) {
				if (ImGui::Button("Kill game"))
					C.gameManager->KillGame();

				ImGui::SameLine();

				if (ImGui::Button("Inject")) {
					std::string modulePath = Utils::GetCurrentModuleDir() + "CarbonSupervisor.dll";
					C.gameManager->InjectModule(modulePath);
				}
			}
			else {
				if (ImGui::Button("Start game"))
					C.gameManager->StartGame();
			}

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
