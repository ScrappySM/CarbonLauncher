#include "pch.h"

#include "window.h"
#include "helpers.h"
#include "font/IconsFontAwesome6.h"

static void RenderDiscover(State* state) {
	ZoneScoped;

	ImGui::SeparatorText("Discover");
}

static void RenderConsole(State* state) {
	ZoneScoped;

	ImGui::SeparatorText("Console");
}

static void RenderSettings(State* state) {
	ZoneScoped;

	ImGui::SeparatorText("Settings");

	static float width = (ImGui::GetWindowContentRegionMax().x - (ImGui::GetStyle().FramePadding.x + (ImGui::GetStyle().WindowPadding.x * 2))) / 2;

	ImGui::BeginChild("GUI options", ImVec2(width, 0), true);

	static bool* idlingEnabled = state->idler.GetEnableIdling();
	ImGui::Checkbox("Idling enabled", idlingEnabled);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ICON_FA_INFO);
	ImGui::SetItemTooltip("Whether the launcher's GUI should limit it's FPS when not interacted with, this is a performance feature");

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Game options", ImVec2(width, 0), true);

	static bool tmp = true;
	ImGui::Checkbox("Inject CarbonSupervisor", &tmp);
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ICON_FA_INFO);
	ImGui::SetItemTooltip("Whether the launcher should inject the CarbonSupervisor into the game after launch, it must be injected manually, only disable this if you know what you are doing!");

	ImGui::EndChild();
}

static void Render(Window& window) {
	ZoneScoped;

	static State* state = State::GetInstance();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2((float)window.width, (float)window.height));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Carbon Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNav);

	// Control
	{
		ImGui::BeginChild("##Control", ImVec2(0, 64), true);

		static Managers::Game* gameManager = &state->gameManager;
		bool isGameRunning = state->gameManager.IsRunning();

		const char* text = isGameRunning ? "Kill game" : "Launch game";
		if (ImGui::Button(text, ImVec2(128, 48))) {
			isGameRunning ? gameManager->KillGame() : gameManager->LaunchGame();
		}

		ImGui::EndChild();
	}

	// Tab Bar
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

		ImGui::BeginChild("##TabBar", ImVec2(64, 0), true);

		RenderTab(Tab::Discover, ICON_FA_PUZZLE_PIECE, "Mods");
		RenderTab(Tab::Console, ICON_FA_TERMINAL, "Console");
		RenderTab(Tab::Settings, ICON_FA_GEARS, "Settings");

		ImGui::EndChild();
	}

	// Content
	{
		ImGui::SameLine();

		ImGui::BeginChild("##Content", ImVec2(0, 0), true);

		switch (state->currentTab) {
		case Tab::Discover:
			RenderDiscover(state);
			break;
		case Tab::Console:
			RenderConsole(state);
			break;
		case Tab::Settings:
			RenderSettings(state);
			break;
		}

		ImGui::EndChild();
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(void) {
	ZoneScoped;

	Window window("Carbon Launcher", 1280, 720);

	static State* state = State::GetInstance();
	static auto clearColour = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

	while (!glfwWindowShouldClose(window.GetContext())) {
		ZoneScopedN("Main Loop");

		state->idler.IdleBySleeping();

		glfwPollEvents();

		glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
		glClear(GL_COLOR_BUFFER_BIT);

		Render(window);

		glfwSwapBuffers(window.GetContext());

		static ImGuiIO& io = ImGui::GetIO();
		TracyPlot("FPS", io.Framerate);

		TracyGpuCollect;
		FrameMark;
	}

	return 0;
}