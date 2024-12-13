#pragma once

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE

#include <Windows.h>

#include <functional>

#include <GLFW/glfw3.h>

namespace Carbon {
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
	};
}; // namespace Carbon

void _GUI();
