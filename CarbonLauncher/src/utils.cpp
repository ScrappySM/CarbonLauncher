#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32

#include "utils.h"

#include <Windows.h>
#include <Tlhelp32.h>
#include <shellapi.h>

#include <filesystem>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <functional>
#include <string>

#undef CreateWindow

#include <spdlog/spdlog.h>

/*
 * Get the last error message as a string.
 *
 * @return The last error message as a string.
 */
std::string GetLastErrorAsString() {
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return std::string();
	}
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
}

/*
 * Get the process ID of a process by name.
 *
 * @param processName The name of the process to get the ID of.
 * @return The process ID of the process with the given name, or 0 if the process was not found.
 */
DWORD GetProcID(const std::string& processName) {
	PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (snapshot == INVALID_HANDLE_VALUE) {
		spdlog::error("Failed to create snapshot: {}", GetLastErrorAsString());
		return 0;
	}

	if (!Process32First(snapshot, &processEntry)) {
		spdlog::error("Failed to get first process: {}", GetLastErrorAsString());
		CloseHandle(snapshot);
		return 0;
	}

	while (Process32Next(snapshot, &processEntry)) {
		if (processName == processEntry.szExeFile) {
			CloseHandle(snapshot);
			return processEntry.th32ProcessID;
		}
	}

	CloseHandle(snapshot);
	return 0;
}

/*
 * Inject a DLL into a process.
 *
 * @param targetPID The process ID of the target process.
 * @param window The window to set as the foreground window after injection.
 * @return True if the DLL was successfully injected, false otherwise.
 */
[[nodiscard]] bool Inject(DWORD targetPID, HWND window, const std::string& toLoad) {
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
	if (process == NULL) {
		spdlog::error("Failed to open process: {}", GetLastErrorAsString());
		return false;
	}

	//char buffer[MAX_PATH];
	//GetModuleFileName(NULL, buffer, MAX_PATH);
	//std::string dllPath = std::string(buffer, strlen(buffer));
	//dllPath = dllPath.substr(0, dllPath.find_last_of('\\') + 1) + toLoad;

	std::string dllPath = toLoad;

	if (!std::filesystem::exists(dllPath)) {
		spdlog::error("DLL does not exist: {}", dllPath);
		CloseHandle(process);
		return false;
	}

	spdlog::info("Injecting DLL: {}", dllPath);

	LPVOID remoteMemory = VirtualAllocEx(process, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (remoteMemory == NULL) {
		spdlog::error("Failed to allocate memory in target process: {}", GetLastErrorAsString());
		CloseHandle(process);
		return false;
	}

	if (!WriteProcessMemory(process, remoteMemory, dllPath.c_str(), dllPath.size() + 1, NULL)) {
		spdlog::error("Failed to write memory in target process: {}", GetLastErrorAsString());
		CloseHandle(process);
		return false;
	}

	HANDLE thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, remoteMemory, 0, NULL);
	if (thread == NULL) {
		spdlog::error("Failed to create remote thread: {}", GetLastErrorAsString());
		CloseHandle(process);
		return false;
	}

	WaitForSingleObject(thread, INFINITE);

	CloseHandle(thread);
	CloseHandle(process);

	return true;
}

/*
 * Enumerate windows callback function.
 *
 * @param hWnd The window handle.
 * @param lParam The process ID to compare against.
 * @return False if the process ID matches, true otherwise.
 */
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
	DWORD pid = 0;
	GetWindowThreadProcessId(hWnd, &pid);

	if (pid == lParam) {
		g_hWnd = hWnd;
		return FALSE;
	}

	return TRUE;
}

std::string GetExeDirectory() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}
	
GLFWwindow* CreateWindow(int width, int height, const char* title) {
	if (!glfwInit()) {
		return nullptr;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!window) {
		glfwTerminate();
		return nullptr;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return nullptr;
	}

	return window;
}


void InitImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Load Segoe UI font
	//io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\CascadiaCode.ttf", 16.0f);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", 18.0f);

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.WindowBorderSize = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.PopupBorderSize = 0.0f;
	style.GrabMinSize = 8.0f;
	style.ChildRounding = 4.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

bool IsGameOpen() {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		return false;
	}
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnap, &pe32)) {
		CloseHandle(hSnap);
		return false;
	}
	do {
		if (strcmp(pe32.szExeFile, "ScrapMechanic.exe") == 0) {
			CloseHandle(hSnap);
			return true;
		}
	} while (Process32Next(hSnap, &pe32));
	CloseHandle(hSnap);
	return false;
}

void Render(GLFWwindow* window, std::function<void()> renderFunc) {
	while (!glfwWindowShouldClose(window)) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		renderFunc();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void OpenGame() {
	ShellExecuteA(NULL, "open", "steam://run/387990/-dev", NULL, NULL, SW_SHOWMINIMIZED);
}

void KillGame() {
	ShellExecuteA(NULL, "open", "taskkill", "/F /IM ScrapMechanic.exe", NULL, SW_SHOWMINIMIZED);
}
