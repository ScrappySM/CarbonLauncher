#pragma once

#include <GLFW/glfw3.h>

#include <functional>

GLFWwindow* CreateWindow(int width, int height, const char* title);
void InitImGui(GLFWwindow* window);

void Render(GLFWwindow* window, std::function<void()> renderFunc);

bool IsGameOpen();

void OpenGame();
void KillGame();
