#pragma once

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE

#include <Windows.h>

#include <functional>

#include <GLFW/glfw3.h>

namespace Carbon {
	enum LogColour : WORD {
		DARKGREEN = 2,
		BLUE = 3,
		PURPLE = 5,
		GOLD = 6,
		WHITE = 7,
		DARKGRAY = 8,
		DARKBLUE = 9,
		GREEN = 10,
		CYAN = 11,
		RED = 12,
		PINK = 13,
		YELLOW = 14,
	};

	class GUIManager {
	public:
		// Initializes the GUI manager
		GUIManager();
		~GUIManager();

		// Sets the render callback (called every frame on the main thread)
		// @param callback The callback to call
		// @note This is where you should put your ImGui code
		void RenderCallback(std::function<void()> callback) {
			this->renderCallback = callback;
		}

		// Runs the GUI manager
		// @note This function will block until the window is closed
		void Run() const;

		GLFWwindow* window;
		std::function<void()> renderCallback;

		enum class Tab {
			MyMods,
			Discover,
			Console,
			Settings,
		} tab{ Tab::MyMods };
	};
}; // namespace Carbon

void _GUI();
