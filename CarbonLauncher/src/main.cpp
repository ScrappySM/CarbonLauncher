#include "pch.h"

#include "window.h"
#include "helpers.h"
#include "state.h"
#include "idler.h"
#include "managers/game.h"
#include "managers/repo.h"

#include "font/IconsFontAwesome6.h"

using namespace CL;

static void RenderDiscover() {
	ImGui::SeparatorText("Discover");

	static char jsonBuffer[256] = "https://raw.githubusercontent.com/ScrappySM/CarbonRepo/refs/heads/main/wip.json";
	ImGui::InputText("##JSON", jsonBuffer, sizeof(jsonBuffer));

	ImGui::SameLine();

	static bool ret = false;
	if (ImGui::SmallButton("Load")) {
		ret = RepoManager::GetInstance().LoadRepo(jsonBuffer);
	}

	if (ret) {
		ImGui::ItemSize(ImVec2(0, 16));

		if (ImGui::TreeNode("Mods")) {
			auto& mods = RepoManager::GetInstance().GetMods();
			for (auto& mod : mods) {
				if (ImGui::TreeNode(mod.name)) {
					ImGui::Text("Description: %s", mod.description);
					ImGui::Text("Commit Hash: %s", mod.commitHash);

					if (ImGui::TreeNode("Authors")) {
						for (auto& author : mod.authors) {
							ImGui::Text(author.c_str());
						}

						ImGui::TreePop();
					}

					if (ImGui::TreeNode("Files")) {
						for (auto& file : mod.files) {
							if (ImGui::TreeNode(file.name)) {
								ImGui::Text("URL: %s", file.url);
								ImGui::Text("Hash: %s", file.hash);
								ImGui::TreePop();
							}
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
}

static void RenderConsole() {
	ImGui::SeparatorText("Console");
}

static void RenderSettings() {
	ImGui::SeparatorText("Settings");
}

static void Render(Window& window) {
	static State& state = State::GetInstance();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2((float)window.width, (float)window.height));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Carbon Launcher", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNav);

	// Control
	{
		ImGui::BeginChild("##Control", ImVec2(0, 64), true);

		static GameManager& gameManager = GameManager::GetInstance();
		bool isGameRunning = gameManager.IsRunning();

		const char* text = isGameRunning ? "Kill game" : "Launch game";
		if (ImGui::Button(text, ImVec2(128, 48))) {
			isGameRunning ? gameManager.KillGame() : gameManager.LaunchGame();
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

		switch (state.currentTab) {
		case Tab::Discover:
			RenderDiscover();
			break;
		case Tab::Console:
			RenderConsole();
			break;
		case Tab::Settings:
			RenderSettings();
			break;
		}

		ImGui::EndChild();
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(void) {
	Window window("Carbon Launcher", 1280, 720);

	static State& state = State::GetInstance();
	static auto clearColour = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

	Idler idler;
	while (!glfwWindowShouldClose(window.GetContext())) {
		idler.IdleBySleeping();

		glfwPollEvents();

		glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
		glClear(GL_COLOR_BUFFER_BIT);

		Render(window);
		glfwSwapBuffers(window.GetContext());
	}

	return 0;
}