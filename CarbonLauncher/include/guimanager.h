#pragma once

#define WIN32_LEAN_AND_MEAN
#define GLFW_INCLUDE_NONE

#include <Windows.h>

#include <functional>

#include <GLFW/glfw3.h>

namespace Carbon {
	class GUIManager {
	public:
		GUIManager(HINSTANCE hInstance);
		~GUIManager();

		void RenderCallback(std::function<void()> callback) {
			this->renderCallback = callback;
		}

		void Run() const;

		GLFWwindow* window;
		std::function<void()> renderCallback;
	};
}; // namespace Carbon

void _GUI();
