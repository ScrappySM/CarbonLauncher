#pragma once

#include <GLFW/glfw3.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <string>

#undef CreateWindow

static HWND g_hWnd = nullptr;

/*
 * Get the last error message as a string.
 *
 * @return The last error message as a string.
 */
std::string GetLastErrorAsString();

/*
 * Get the process ID of a process by name.
 *
 * @param processName The name of the process to get the ID of.
 * @return The process ID of the process with the given name, or 0 if the process was not found.
 */
DWORD GetProcID(const std::string& processName);

/*
 * Inject a DLL into a process.
 *
 * @param targetPID The process ID of the target process.
 * @param window The window to set as the foreground window after injection.
 * @return True if the DLL was successfully injected, false otherwise.
 */
[[nodiscard]] bool Inject(DWORD targetPID, HWND window, const std::string& toLoad);

/*
 * Enumerate windows callback function.
 *
 * @param hWnd The window handle.
 * @param lParam The process ID to compare against.
 * @return False if the process ID matches, true otherwise.
 */
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);

std::string GetExeDirectory();

GLFWwindow* CreateWindow(int width, int height, const char* title);
void InitImGui(GLFWwindow* window);

void Render(GLFWwindow* window, std::function<void()> renderFunc);

bool IsGameOpen(const char* name);
